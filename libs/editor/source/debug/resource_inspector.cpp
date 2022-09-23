#include "hades/debug/resource_inspector.hpp"

#include "SFML/Graphics/Image.hpp"

#include "ImGuiFileDialog.h"

#include "hades/animation.hpp"
#include "hades/data.hpp"
#include "hades/gui.hpp"
#include "hades/sf_color.hpp"
#include "hades/sf_streams.hpp"
#include "hades/texture.hpp"
#include "hades/utility.hpp"
#include "hades/writer.hpp"

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace hades::data
{
	class animation_editor : public resource_editor
	{
	public:
		void set_target(data_manager& d, unique_id i, unique_id mod) override
		{
			namespace anim = resources::animation_functions;
			_anim = anim::get_resource(d, i, mod);
			_name = "Animation: "s + d.get_as_string(i) + "###anim-edit"s;
			const auto texture = anim::get_texture(*_anim);
			if (texture)
			{
				const auto tex_id = resources::texture_functions::get_id(texture);
				_tex_name = d.get_as_string(tex_id);
			}

			_duration = anim::get_duration(*_anim);
			_base = d.get_resource(i);

			// size of first frame
			_anim_frames = anim::get_animation_frames(*_anim);
			if (!empty(_anim_frames))
			{
				const auto& first = _anim_frames.front();
				_size = { abs(first.w * first.scale_w), abs(first.h * first.scale_h) };
			}

			return;
		}

		void update(data::data_manager& d, gui& g) override
		{
			namespace anim = resources::animation_functions;

			const auto generate_yaml = [this](data::data_manager& d) {
				auto writer = data::make_writer();
				_base->serialise(d, *writer);
				_yaml = writer->get_string();
			};

			if (empty(_yaml))
				generate_yaml(d);

			g.next_window_size({}, gui::set_condition_enum::first_use);
			if (g.window_begin(_name, gui::window_flags::no_collapse))
			{
				if (!editable(_base->mod))
					g.begin_disabled();

				g.begin_disabled();
				g.input("Texture"sv, _tex_name);
				g.end_disabled();

				auto ms = time_cast<milliseconds>(_duration).count();
				if (g.input_scalar("Duration(ms)"sv, ms))
				{
					_duration = time_cast<time_duration>(milliseconds{ ms });
					anim::set_duration(*_anim, _duration);
					generate_yaml(d);
				}

				if (!editable(_base->mod))
					g.end_disabled();

				g.text("Resource as YAML:"sv);
				g.input_text_multiline("##yaml"sv, _yaml, {}, gui::input_text_flags::readonly);
			}

			g.window_end();

			g.next_window_size({}, gui::set_condition_enum::first_use);
			if (g.window_begin("Animation frames"sv, gui::window_flags::no_collapse))
			{
				if (!editable(_base->mod))
					g.begin_disabled();

				auto mod = false;
				g.text("Frames:"sv);
				g.layout_horizontal();
				if (g.button("Add frame"sv))
				{
					_anim_frames.emplace_back();
					mod = true;
				}

				if (g.child_window_begin("anim-frames"sv))
				{
					const auto begin = std::begin(_anim_frames);
					const auto end = std::end(_anim_frames);
					auto frame_count = 0;
					auto delete_frame = end;
					for (auto iter = begin; iter != end; ++iter)
					{
						g.push_id(&*iter);
						g.text("Frame: "s + to_string(frame_count++));
						g.layout_horizontal();
						if (iter == begin)
							g.begin_disabled();
						if (g.arrow_button("up"sv, gui::direction::up))
						{
							mod = true;
							std::iter_swap(iter, prev(iter));
						}
						if (iter == begin)
							g.end_disabled();

						auto is_end = next(iter) == end;

						g.layout_horizontal();

						if (is_end)
							g.begin_disabled();
						if (g.arrow_button("down"sv, gui::direction::down))
						{
							mod = true;
							std::iter_swap(iter, next(iter));
						}

						if (is_end)
							g.end_disabled();

						g.layout_horizontal();
						if (g.button("Remove Frame"sv))
							delete_frame = iter;

						auto pos = std::array{ iter->x, iter->y };
						if (g.input_scalar("Position [x, y]"sv, pos))
						{
							mod = true;
							iter->x = pos[0];
							iter->y = pos[1];
						}

						auto siz = std::array{ iter->w, iter->h };
						if (g.input_scalar("Size [w, h]"sv, siz))
						{
							mod = true;
							iter->w = siz[0];
							iter->h = siz[1];
						}

						auto off = std::array{ iter->off_x, iter->off_y };
						if (g.input_scalar("Position Offset [x, y]"sv, off))
						{
							mod = true;
							iter->off_x = off[0];
							iter->off_y = off[1];
						}

						auto scale = std::array{ iter->scale_w, iter->scale_h };
						if (g.input_scalar("Scale [w, h]"sv, scale))
						{
							mod = true;
							iter->scale_w = scale[0];
							iter->scale_h = scale[1];
						}

						if (g.input_scalar("Relative duration"sv, iter->duration))
							mod = true;

						if (!is_end)
							g.separator_horizontal();
						g.pop_id();
					}

					if (delete_frame != end)
					{
						_anim_frames.erase(delete_frame);
						mod = true;
					}

					if (mod)
					{
						anim::set_animation_frames(*_anim, _anim_frames);
						generate_yaml(d);
					}
				}
				g.child_window_end();

				if (!editable(_base->mod))
					g.end_disabled();
			}
			g.window_end();

			g.next_window_size({}, gui::set_condition_enum::first_use);
			if (g.window_begin("Animation preview", gui::window_flags::no_collapse))
			{
				g.slider_float("Scale"sv, _scale, 0.5f, 50.f, gui::slider_flags::logarithmic);
				if (!_play)
					g.begin_disabled();
				g.slider_float("Speed"sv, _play_speed, 0.1f, 5.f);
				if (g.is_item_hovered())
					g.tooltip("Ctrl + Click to enter value"sv);
				if (!_play)
					g.end_disabled();
				
				g.checkbox("Play animation"sv, _play);
				if (_play)
					g.begin_disabled();
				g.slider_int("Frame"s, _current_frame, 0, std::max(integer_cast<int>(size(_anim_frames)) - 1, 0), gui::slider_flags::no_input);
				if (_play)
					g.end_disabled();
				if (g.child_window_begin("##anim-preview"sv, {}, false, gui::window_flags::horizontal_scrollbar))
				{
					if (integer_cast<std::size_t>(_current_frame) < size(_anim_frames))
					{
						const auto float_size = gui::vector2{ _size[0], _size[1] };
						if (_play)
						{
							const auto& io = ImGui::GetIO();
							_time += io.DeltaTime * _play_speed;
							const auto sec = seconds_float{ _time };
							const auto time = time_point{ time_cast<time_duration>(sec) };
							g.image(*_anim, float_size * _scale, time, sf::Color::White, sf::Color::White);

							//select correct frame in ui
							const auto& frame = animation::get_frame(*_anim, time);
							const auto& frames = anim::get_animation_frames(*_anim);
							for (auto i = std::size_t{}; i < size(frames); ++i)
							{
								if (&frame == &frames[i])
									_current_frame = integer_cast<int>(i);
							}
						}
						else
						{
							const auto iter = next(begin(_anim_frames), _current_frame);
							_time = std::transform_reduce(begin(_anim_frames), iter,
								0.f, std::plus<>(), std::mem_fn(&resources::animation_frame::normalised_duration));
							_time *= time_cast<seconds_float>(_duration).count(); // -> to seconds first

							// Add epsilon to tie break frames. Otherwise we err towards frames towards the start of the animation
							// when we have an even number of frames, this means half the frames are inaccessible.
							const auto sec = seconds_float{ _time + std::numeric_limits<float>::epsilon() };
							const auto time = time_cast<time_duration>(sec);// *_duration.count();
							g.image(*_anim, float_size * _scale, time_point{ time }, sf::Color::White, sf::Color::White);
						}
					}//! if currentframe < size
				}
				g.child_window_end();
			}

			g.window_end();
			return;
		}

	private:
		resources::animation* _anim = {};
		resources::resource_base* _base = {};
		time_duration _duration;
		int _current_frame = {};
		std::array<float, 2> _size;
		std::vector<resources::animation_frame> _anim_frames;
		string _name;
		string _tex_name;
		string _yaml;
		float _scale = 1.f;
		float _time = 0.f;
		float _play_speed = 1.f;
		bool _play = false;
	};

	class texture_editor : public resource_editor
	{
	public:
		void set_target(data_manager& d, unique_id i, unique_id mod) override
		{
			namespace tex = resources::texture_functions;
			_texture = tex::get_resource(d, i, mod);
			_texture_base = tex::get_resource_base(*_texture);
			_name = "Texture: "s + d.get_as_string(i) + "###texture-edit"s;
			_smooth = tex::get_smooth(_texture);
			_mips = tex::get_mips(_texture);
			const auto a = tex::get_alpha_colour(*_texture);
			_alpha_set = a.has_value();
			if (_alpha_set)
				_alpha = { a->r, a->g, a->b };

			_repeat = tex::get_repeat(_texture);
			_source = _texture_base->source.generic_string();
			const auto size = tex::get_size(*_texture);
			_size[0] = size.x;
			_size[1] = size.y;
			const auto requested_size = tex::get_requested_size(*_texture);
			_requested_size[0] = requested_size.x;
			_requested_size[1] = requested_size.y;

			_data_man = &d;
			_file_dialog.SetCreateThumbnailCallback([this](IGFD_Thumbnail_Info* data) {
				if (data && data->isReadyToUpload && data->textureFileDatas)
				{
					auto id = make_unique_id();
					auto tex = resources::texture_functions::find_create_texture(*_data_man, id);
					static_assert(sizeof(unique_id) >= sizeof(void*));
					assert(tex);
					auto image = sf::Image{};
					image.create(data->textureWidth, data->textureHeight, reinterpret_cast<sf::Uint8*>(data->textureFileDatas));
					resources::texture_functions::load_from_image(*tex, image);
					data->textureID = tex;
					delete[] data->textureFileDatas;
					data->textureFileDatas = {};
					data->isReadyToUpload = false;
					data->isReadyToDisplay = true;
				}
				return;
				});

			_file_dialog.SetDestroyThumbnailCallback([this](IGFD_Thumbnail_Info* data) {
				if (data && data->textureID)
				{
					auto tex = reinterpret_cast<const resources::texture*>(data->textureID);
					_data_man->erase(resources::texture_functions::get_id(tex));
				}
				return;
				});

			return;
		}

		void update(data::data_manager& d,gui& g) override
		{
			// we can call this here, because we
			// use a single threaded OGLrendering paths
			_file_dialog.ManageGPUThumbnails();
			const auto generate_yaml = [this](data::data_manager& d) {
				auto writer = data::make_writer();
				_texture_base->serialise(d, *writer);
				_yaml = writer->get_string();
			};

			if (empty(_yaml))
				generate_yaml(d);

			if (g.window_begin(_name, gui::window_flags::no_collapse))
			{
				if (!editable(_texture_base->mod))
					g.begin_disabled();

				auto mod = false;
				mod |= g.checkbox("Smooth"sv, _smooth)
					| g.checkbox("Repeating"sv, _repeat)
					| g.checkbox("Mip mapping"sv, _mips)
					| g.input_scalar("Requested size"sv, _requested_size);

				g.separator_horizontal();

				assert(_alpha_combo_index < size(colours::all_colour_names));
				if (g.combo_begin("Alpha colour"sv, colours::all_colour_names[_alpha_combo_index], gui::combo_flags::no_preview))
				{
					for (auto i = std::size_t{}; i < size(colours::all_colour_names); ++i)
					{
						if (g.selectable(colours::all_colour_names[i], _alpha_combo_index == i))
						{
							_alpha_combo_index = i;
							assert(_alpha_combo_index < size(colours::all_colours));
							const auto& col = colours::all_colours[i];
							_alpha = { col.r, col.g, col.b };
							_alpha_set = true;
							mod = true;
						}
					}
					g.combo_end();
				}

				if (g.input_scalar("Alpha rgb"sv, _alpha)
					|| g.button("Remove custom alpha"sv))
				{
					_alpha_set = true;
					mod = true;
				}

				g.separator_horizontal();

				g.begin_disabled();
				g.input_scalar("Raw size"sv, _size, {}, {});
				g.end_disabled();
				g.input_text("##Source"sv, _source);
				g.same_line();
				if (g.button("Source..."))
				{
					const auto& [arc_path, path] = resources::texture_functions::get_loaded_paths(*_texture);
					_texture_source_window = source_window{ true, _source, arc_path.generic_string(), path.generic_string()};
				}
				
				if (mod)
				{
					if (_alpha_set)
						resources::texture_functions::set_alpha_colour(*_texture, { _alpha[0], _alpha[1], _alpha[2] });
					else
						resources::texture_functions::clear_alpha(*_texture);

					resources::texture_functions::set_settings(_texture, { _requested_size[0], _requested_size[1] },
						_smooth, _repeat, _mips, false);
					resources::texture_functions::get_resource(d, _texture_base->id, _texture_base->mod);
					const auto size = resources::texture_functions::get_size(*_texture);
					_size[0] = size.x;
					_size[1] = size.y;
					generate_yaml(d);
				}

				if (!editable(_texture_base->mod))
					g.end_disabled();

				g.text("Resource as YAML:"sv);
				g.input_text_multiline("##yaml"sv, _yaml, {}, gui::input_text_flags::readonly);
			}
			
			g.window_end();
			
			if (g.window_begin("Texture preview", gui::window_flags::no_collapse))
			{
				g.slider_float("Scale"sv, _scale, 0.5f, 50.f, gui::slider_flags::logarithmic);
				if (g.child_window_begin("##tex-preview"sv, {}, false, gui::window_flags::horizontal_scrollbar))
				{
					const auto float_size = gui::vector2{ float_cast(_size[0]), float_cast(_size[1]) };
					ImVec2 pos = ImGui::GetCursorScreenPos();
					g.image(*_texture, { {}, float_size }, float_size * _scale);

					// pixel zoom tooltip
					if (g.is_item_hovered())
					{
						const auto& io = ImGui::GetIO();

						const auto& sf_tex = resources::texture_functions::get_sf_texture(_texture);
						const auto img = sf_tex.copyToImage();
						const auto img_size = img.getSize();

						const auto region_pos = gui::vector2{ ((io.MousePos.x - pos.x) / _scale),
											 ((io.MousePos.y - pos.y) / _scale) };
						
						const auto region_uint = vector_t<unsigned int>{
							integral_cast<unsigned int>(region_pos.x, round_down_tag),
							integral_cast<unsigned int>(region_pos.y, round_down_tag)
						};
						
						const auto col = from_sf_color(img.getPixel(std::min(img_size.x - 1, region_uint.x),
							std::min(img_size.y - 1, region_uint.y)));

						g.tooltip_begin();

						g.text("Texel coord: ["s + to_string(region_pos.x) + ", "s + to_string(region_pos.y) + "]"s);
						const auto size1 = g.get_item_rect_size();
						g.text("Colour: "s + to_string(col));
						const auto size2 = g.get_item_rect_size();
						const auto size = ImVec2{ std::max(size1.x, size2.x), size1.y + size2.y };

						auto drawlist = ImGui::GetWindowDrawList();
						const auto im_col = ImGui::GetColorU32(IM_COL32(col.r, col.g, col.b, col.a));
						const auto rect_pos = ImGui::GetCursorScreenPos();
						drawlist->AddRectFilled(rect_pos, { rect_pos.x + size.x, rect_pos.y + size.y }, im_col);
						ImGui::Dummy(size);
						g.tooltip_end();
					}
				}
				g.child_window_end();
			}
			
			g.window_end();

			if (_texture_source_window.open)
			{
				g.next_window_size({}, gui::set_condition_enum::first_use);
				if (g.window_begin("Choose texture source",
					_texture_source_window.open, gui::window_flags::no_collapse))
				{
					g.input_text("Resource path"sv, _texture_source_window.source);
					g.begin_disabled();
					if (!_texture_source_window.raw_archive.empty())
					{
						g.input_text("##archive_path"sv, _texture_source_window.raw_archive);
						g.same_line();
					}
					g.input_text("##src_path"sv, _texture_source_window.raw_path);
					g.end_disabled();
					g.same_line();
					if (g.button("Choose new file"sv))
						_make_file_dialog();

					using fs_path = std::filesystem::path;
					const auto good_ext = fs_path{ _texture_source_window.source }.extension()
						== fs_path{ _texture_source_window.raw_path }.extension();

					const auto applyable = good_ext && (_texture_source_window.raw_archive == string{} ||
						_texture_source_window.source != _source);

					if (!good_ext)
						g.text_coloured("file extensions must match"sv, colours::red);

					if (!applyable)
							g.begin_disabled();

					if (g.button("Apply"sv))
					{
						if (_texture_source_window.raw_archive == string{})
						{
							namespace tex = resources::texture_functions;
							if (tex::load_from_file(*_texture, _texture_source_window.raw_path))
							{
								const auto size = tex::get_size(*_texture);
								_size[0] = size.x;
								_size[1] = size.y;
								const auto requested_size = tex::get_requested_size(*_texture);
								_requested_size[0] = requested_size.x;
								_requested_size[1] = requested_size.y;
							}
						}

						if (_source != _texture_source_window.source)
						{
							_texture_base->source = _texture_source_window.source;
							_source = _texture_base->source.generic_string();
						}
					}

					if (!applyable)
						g.end_disabled();

					g.same_line();
					g.button("Cancel"sv);
				}

				g.window_end();
			}

			if (_file_dialog.Display("texture_dialog"s))
			{
				if (_file_dialog.IsOk())
				{
					_texture_source_window.raw_archive = {};
					_texture_source_window.raw_path = _file_dialog.GetFilePathName();
				}

				_file_dialog.Close();
			}

			return;
		}

	private:
		void _make_file_dialog()
		{
			constexpr auto supported_image_file_types = 
				"Image Files (*.bmp;*.dds;*.jpg;*.png;*.tga;*.psd){.bmp,.dds,.jpg,.jpeg,.png,.tga,.psd}";
			_file_dialog.OpenDialog("texture_dialog"s, "Image"s, supported_image_file_types, "."s, 1, {}, ImGuiFileDialogFlags_DisableCreateDirectoryButton);
		}

		struct source_window
		{
			bool open = false;
			string source; 
			string raw_archive;
			string raw_path;
		};

		data::data_manager* _data_man = {};
		resources::resource_base* _texture_base = {};
		resources::texture* _texture = {};
		string _name;
		string _yaml;
		string _source;
		bool _smooth;
		bool _mips;
		bool _repeat;
		float _scale = 1.f;
		source_window _texture_source_window;
		ImGuiFileDialog _file_dialog;

		std::array<uint8, 3> _alpha;
		bool _alpha_set = false;
		std::size_t _alpha_combo_index = std::size_t{};
		std::array<texture_size_t, 2> _requested_size;
		std::array<texture_size_t, 2> _size;
	};

	class animation_group_editor : public resource_editor
	{
	public:
		void set_target(data_manager& d, unique_id i, unique_id mod) override
		{
			_group = resources::animation_group_functions::get_resource(d, i, mod);
			_base = d.get_resource(i, mod);

			assert(_group);
			const auto anims = resources::animation_group_functions::get_all_animations(*_group);

			for (const auto& [name, anim] : anims)
				_anim_list.emplace_back(d.get_as_string(name), d.get_as_string(anim));

			_title = "animation group: "s + d.get_as_string(i) + "###anim_group_win"s;

			return;
		}

		void update(data::data_manager& d, gui& g) override
		{
			auto text_callback = [this, &d](gui_input_text_callback& input) {
				if (empty(_completion_list))
				{
					const auto current = input.input_contents();
					_completion_index = {};
					auto resource_list = d.get_all_names_for_type("animations"sv);
					resource_list.erase(std::remove_if(begin(resource_list), end(resource_list), [current](const std::string_view& elm) {
						return elm.find(current) == std::string_view::npos;
						}), end(resource_list));

					std::partition(begin(resource_list), end(resource_list), [current](auto name) {
						return name.find(current) == std::string_view::size_type{};
						});

					_completion_list = std::move(resource_list);
					if (!empty(_completion_list))
					{
						input.clear_chars();
						input.insert_chars(0, _completion_list[_completion_index]);
					}
				}
				else
				{
					input.clear_chars();
					if (++_completion_index >= size(_completion_list))
						_completion_index = {};
					input.insert_chars(0, _completion_list[_completion_index]);
				}
			};

			g.next_window_size({}, gui::set_condition_enum::first_use);
			if (g.window_begin(_title))
			{
				if (!editable(_base->mod))
					g.begin_disabled();

				using tflags = gui::table_flags;
				if (g.begin_table("things"sv, 3, tflags::sortable | tflags::borders
					| tflags::sizing_stretch_same))
				{
					using flags = gui::table_column_flags;
					g.table_setup_column("Name"sv, {}, 3.f);
					g.table_setup_column("Animation"sv, flags::no_sort, 3.f);
					g.table_setup_column("Remove row", flags::no_sort | flags::no_header_label |flags::no_header_width, 1.f);
					g.table_headers_row();

					if (g.table_needs_sort())
					{
						auto [column, sort_order] = g.table_sort_column(0);
						assert(column == 0);
						switch (sort_order)
						{
						case gui::sort_direction::ascending:
							std::ranges::sort(_anim_list, {}, &entry_t::name);
							break;
						case gui::sort_direction::descending:
							std::ranges::sort(_anim_list, std::ranges::greater{}, &entry_t::name);
							break;
						case gui::sort_direction::none:
							; // TODO: there doesn't seem to be a way to set this as the default in ImGui with tri-state?
						}
					}

					auto changed = false;
					auto remove = end(_anim_list);

					const auto end = std::end(_anim_list);
					for(auto iter = begin(_anim_list); iter != end; ++iter)
					{
						auto& [name, value] = *iter;
						g.table_next_row();
						if (g.table_next_column())
						{
							g.push_id(&name);
							g.push_item_width(-1.f);

							const auto was_invalid = std::ranges::count(_anim_list, name, &entry_t::name) > 1;
							if (was_invalid)
								g.push_colour(gui::colour_target::frame_background, colours::dark_red);
							// must be unique
							if(g.input("##name"sv, name) 
								&& std::ranges::count(_anim_list, name, &entry_t::name) == 1)
								changed = true;
							if (was_invalid)
								g.pop_colour();

							g.pop_item_width();
							g.pop_id();
						}

						if (g.table_next_column())
						{
							g.push_id(&name);
							g.push_item_width(-1.f);

							const auto anim_names = d.get_all_names_for_type("animations"sv);
							const auto was_invalid = std::ranges::none_of(anim_names, [&value](const auto& str)->bool {
									return str == value;
								});

							if (was_invalid)
								g.push_colour(gui::colour_target::frame_background, colours::dark_red);
							if (g.input_text("##value"sv, value, gui::input_text_flags::callback_completion, text_callback)
								&& std::ranges::any_of(anim_names, [&value](const auto& str)->bool{
									return str == value;
									}))
							{
								changed = true;
							}
							if (was_invalid)
								g.pop_colour();

							if (g.is_item_focused() && _last_focused != &value)
							{
								_last_focused = &value;
								_completion_list = {};
							}
							g.pop_item_width();
							g.pop_id();
						}

						if (g.table_next_column())
						{
							g.push_id(&name);
							if (g.button("remove", {-1.f, 0.f}))
								remove = iter;
							
							g.pop_id();
						}
					}

					if (remove != end)
					{
						_anim_list.erase(remove);
						changed = true;
					}

					if (changed)
					{
						// update resource
						auto new_values = std::unordered_map<unique_id, resources::resource_link<resources::animation>>{};
						new_values.reserve(size(_anim_list));
						for (const auto& [name, value] : _anim_list)
						{
							new_values.emplace(d.get_uid(name),
								resources::animation_functions::make_resource_link(d, d.get_uid(value), _base->id));
						}

						resources::animation_group_functions::replace_animations(*_group, std::move(new_values));
					}

					g.end_table();
				}

				if (g.button("Add row"sv))
					_anim_list.emplace_back();

				if (!editable(_base->mod))
					g.end_disabled();
			}
			g.window_end();
		}

	private:
		struct entry_t
		{
			string name;
			string animation;
		};

		std::vector<entry_t> _anim_list;
		std::vector<std::string_view> _completion_list;
		string _title;
		std::string* _last_focused = {};
		std::size_t _completion_index;
		resources::animation_group* _group = {};
		const resources::resource_base* _base = {};
	};

	void basic_resource_inspector::update(gui& g, data::data_manager& d)
	{
		_resource_tree(g, d);

		if (_tree_state.res_editor)
			_tree_state.res_editor->update(d, g);
		return;
	}

	std::unique_ptr<resource_editor> basic_resource_inspector::make_resource_editor(std::string_view s)
	{
		if (s == "textures"sv)
			return std::make_unique<texture_editor>();
		else if (s == "animations"sv)
			return std::make_unique<animation_editor>();
		else if (s == "animation-groups"sv)
			return std::make_unique<animation_group_editor>();

		return {};
	}

	void basic_resource_inspector::_list_resources_from_group(resource_tree_state::group& group, gui& g,
		data::data_manager& d, unique_id mod)
	{
		auto ids = std::vector<unique_id>{};

		for (const auto& [id, name, mod_id, mod_name] : group.resources)
		{
			const auto tree_open = g.tree_node(name, gui::tree_node_flags::leaf);

			if (g.is_item_clicked() && !g.is_item_toggled_open())
			{
				_tree_state.res_editor = make_resource_editor(group.name);

				if (_tree_state.res_editor)
				{
					_tree_state.res_editor->set_target(d, id, mod);
					_tree_state.res_editor->set_editable_mod(_mod);
				}
			}

			if (mod !=  mod_id)
			{
				g.layout_horizontal();
				g.text_disabled(mod_name);
			}
			
			if (tree_open)
				g.tree_pop();
		}
	}

	void basic_resource_inspector::_refresh(data::data_manager& d)
	{
		const auto dat = d.get_mod_stack();
		// build the list of resource types
		auto& g = _tree_state.resource_groups;
		g.clear();

		const auto& mod_index = _tree_state.mod_index;

		const auto stack_end = mod_index >= size(dat) ?
			end(dat) : next(begin(dat), mod_index + 1);
		
		for (auto iter = begin(dat); iter != stack_end; ++iter)
		{
			const auto m = *iter;
			for (const auto& type : m->resources_by_type)
			{
				resource_tree_state::group* group = nullptr;
				auto group_iter = std::find_if(begin(g), end(g), [&](auto&& elm) noexcept {
					return elm.name == type.first;
				});
				
				if (group_iter == end(g))
					group = &g.emplace_back(resource_tree_state::group{ type.first });
				else
					group = &*group_iter;

				using val_type = resource_tree_state::group_val;
				std::transform(begin(type.second), end(type.second), back_inserter(group->resources), [&d, &mod = m->mod_info](auto&& elm)->val_type {
					return { elm->id, d.get_as_string(elm->id), mod.id, mod.name };
				});
			}
		}

		const auto less = [](auto&& left, auto&& right) noexcept {
			return left.id < right.id;
		};

		const auto equal = [](auto&& left, auto&& right) noexcept {
			return left.id == right.id;
		};

		for (auto&& group : g)
		{
			remove_duplicates(group.resources, less, equal);
		}

		std::sort(begin(g), end(g), [](auto&& left, auto&& right) {
			return left.name < right.name;
			});

		return;
	}

	void basic_resource_inspector::_resource_tree(gui& g, data::data_manager& d)
	{
		const auto mods = d.get_mod_stack();
		//main resource list
		// need to set min size manually
		// since the child creates a scrolling region that can fit in a tiny window
		g.next_window_size({}, hades::gui::set_condition_enum::first_use);
		if (g.window_begin("resource tree"sv, gui::window_flags::no_collapse))
		{
			if (_tree_state.mod_index >= size(mods))
			{
				_tree_state.mod_index = size(mods) - 1;
				_refresh(d);
			}

			auto current_mod = _tree_state.mod_index;
			const string preview_str = [&] {
				auto& current = mods[current_mod]->mod_info;
				if (_mod == current.id)
					return current.name + "*"s;
				else
					return current.name;
			}();

			if (g.combo_begin("Mod"sv, preview_str))
			{
				//list other mod options
				for (auto i = std::size_t{}; i < size(mods); ++i)
				{
					auto& mod_info = mods[i]->mod_info;
					auto str = mod_info.name;
					if (mod_info.id == _mod)
						str += "*"s;

					if (g.selectable(str, i == current_mod))
						_tree_state.mod_index = i;
				}

				g.combo_end();
			}

			if (current_mod != _tree_state.mod_index)
				_refresh(d);

			g.next_window_size({}, hades::gui::set_condition_enum::first_use);
			if (g.child_window_begin("##resource-tree-list"sv))
			{
				g.set_next_item_open(true, gui::set_condition_enum::once);
				if (g.tree_node("resources"sv))
				{
					for (auto& resource_type : _tree_state.resource_groups)
					{
						bool open;
						if (resource_type.name.empty())
							open = g.tree_node("unknown"sv);
						else
							open = g.tree_node(resource_type.name);

						if (open)
						{
							_list_resources_from_group(resource_type, g, d,
								mods[_tree_state.mod_index]->mod_info.id);
							g.tree_pop();
						}
					}

					g.tree_pop();
				}
			}

			g.child_window_end();
		}
		g.window_end();
	}
}
