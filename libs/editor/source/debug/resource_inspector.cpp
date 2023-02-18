#include "hades/debug/resource_inspector.hpp"

#include "SFML/Graphics/Image.hpp"

#include "imgui.h" // for GetID
#include "imgui_internal.h" // for BeginDragDropTargetCustom

#include "ImGuiFileDialog.h"

#include "hades/animation.hpp"
#include "hades/curve_extra.hpp"
#include "hades/data.hpp"
#include "hades/font.hpp"
#include "hades/game_system_resources.hpp"
#include "hades/gui.hpp"
#include "hades/level_editor_objects.hpp"
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
				//force load texture
				resources::texture_functions::get_resource(d, tex_id, mod);
			}

			_duration = anim::get_duration(*_anim);
			_base = d.get_resource(i, mod);

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

		std::string resource_name() const override
		{
			return _name;
		}

		resources::resource_base* get() const noexcept override
		{
			return _base;
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

	// TODO: this class needs to use the source_window class,
	//  rather than duplicating its implementation
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
			else
				_alpha.fill({});

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
                    image.create(unsigned_cast(data->textureWidth), unsigned_cast(data->textureHeight), reinterpret_cast<sf::Uint8*>(data->textureFileDatas));
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

				if (const auto current_alpha = _alpha; 
					g.input_scalar("Alpha rgb"sv, _alpha) && current_alpha != _alpha)
				{
					_alpha_set = true;
					mod = true;
				}

				if(g.button("Remove custom alpha"sv))
				{
					_alpha.fill({});
					_alpha_set = false;
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

		std::string resource_name() const override
		{
			return _name;
		}

		resources::resource_base* get() const noexcept override
		{
			return _texture_base;
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

	struct source_window
	{
		source_window() = default;

		source_window(data::data_manager* data, std::string_view title, std::string_view source,
			std::string_view raw_path, std::string_view raw_archive,
			std::string_view supported_file_types, std::string_view file_type)
            : _data_man{ data }, _source{ source }, _title{ title },
              _dialog_title{ string{ title } + "##dialog" }, _old_source{ source },
              _raw_archive{ raw_archive }, _raw_path{ raw_path },
              _file_types{ supported_file_types }, _file_type_name{ file_type }
		{
			_setup_callbacks();
		}

		source_window& operator=(source_window&& rhs)
		{
			_file_dialog.Close();
			rhs._file_dialog.Close();
			_title = std::move(rhs._title);
			_dialog_title = std::move(rhs._dialog_title);
			_source = std::move(rhs._source);
			_old_source = std::move(rhs._old_source);
			_raw_path = std::move(rhs._raw_path);
			_raw_archive = std::move(rhs._raw_archive);
			_file_types = std::move(rhs._file_types);
			_file_type_name = std::move(rhs._file_type_name);
			_data_man = rhs._data_man;
			_setup_callbacks();
			return *this;
		}
		
		void open()
		{
			_open = true;
			return;
		}

		bool update(gui& g)
		{
			_file_dialog.ManageGPUThumbnails();
			auto ret = false;

			if (_open)
			{
				g.next_window_size({}, gui::set_condition_enum::first_use);
				if (g.window_begin(_title,
					_open, gui::window_flags::no_collapse))
				{
					g.input_text("Resource path"sv, _source);
					g.begin_disabled();
					if (!empty(_raw_archive))
					{
						g.input_text("##archive_path"sv, _raw_archive);
						g.same_line();
					}
					g.input_text("##src_path"sv, _raw_path);
					g.end_disabled();
					g.same_line();
					if (g.button("Choose new file"sv))
						_make_file_dialog();

					using fs_path = std::filesystem::path;
					const auto good_ext = fs_path{ _source }.extension()
						== fs_path{ _raw_path }.extension();

					const auto applyable = good_ext && (_raw_archive == string{} ||
						_source != _old_source);

					if (!good_ext)
						g.text_coloured("file extensions must match"sv, colours::red);

					if (!applyable)
						g.begin_disabled();

					if (g.button("Apply"sv))
						ret = true;

					if (!applyable)
						g.end_disabled();

					g.same_line();
					g.button("Cancel"sv);
				}

				g.window_end();
			}
			else if (_file_dialog.IsOpened())
				_file_dialog.Close();

			if (_file_dialog.Display(_dialog_title))
			{
				if (_file_dialog.IsOk())
				{
					_raw_archive = {};
					_raw_path = _file_dialog.GetFilePathName();
				}

				_file_dialog.Close();
			}

			return ret;
		}

		string get_new_source()
		{
			return std::exchange(_source, _old_source);
		}
		
	private:
		void _setup_callbacks()
		{
			_file_dialog.SetCreateThumbnailCallback([this](IGFD_Thumbnail_Info* data) {
				if (data && data->isReadyToUpload && data->textureFileDatas)
				{
					auto id = make_unique_id();
					auto tex = resources::texture_functions::find_create_texture(*_data_man, id);
					static_assert(sizeof(unique_id) >= sizeof(void*));
					assert(tex);
					auto image = sf::Image{};
                    image.create(unsigned_cast(data->textureWidth), unsigned_cast(data->textureHeight), reinterpret_cast<sf::Uint8*>(data->textureFileDatas));
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
		}
		void _make_file_dialog()
		{
			_file_dialog.OpenDialog(_dialog_title, _file_type_name,
				_file_types.c_str(), "."s, 1, {},
				ImGuiFileDialogFlags_DisableCreateDirectoryButton);
			return;
		}

		data::data_manager* _data_man = {};
		ImGuiFileDialog _file_dialog;
		string _source;
		string _title;
		string _dialog_title;
		string _old_source;
		string _raw_archive;
		string _raw_path;

		string _file_types;
		string _file_type_name;
		bool _open = false;
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

		std::string resource_name() const override
		{
			return _title;
		}

		resources::resource_base* get() const noexcept override
		{
			return _base;
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
		resources::resource_base* _base = {};
	};

	class curve_editor : public resource_editor
	{
	public:
		void set_target(data_manager& d, unique_id i, unique_id mod) override
		{
			_curve = d.get<resources::curve>(i, mod);
			_base = d.get_resource(i, mod);
			_title = "curve: "s + d.get_as_string(i) + "###curve_win"s;

			return;
		}

		void update(data::data_manager& d, gui& g) override
		{
			const auto generate_yaml = [this](data::data_manager& d) {
				auto writer = data::make_writer();
				_base->serialise(d, *writer);
				_yaml = writer->get_string();
			};

			if (empty(_yaml))
				std::invoke(generate_yaml, d);

			g.next_window_size({}, gui::set_condition_enum::first_use);
			if (g.window_begin(_title))
			{
				const auto disabled = !editable(_base->mod);
				auto mod = false;
				if (disabled)
					g.begin_disabled();

				//data type
				if (g.combo_begin("data type"sv, to_string(_curve->data_type)))
				{
					for (auto i = curve_variable_type::begin; i < curve_variable_type::end; i = next(i))
					{
						if (g.selectable(to_string(i), _curve->data_type == i))
						{
							_curve->data_type = i;
							mod = true;
							_curve->default_value = resources::reset_default_value(*_curve);
							_curve_edit_cache = {};
							_curve_vec_edit_cache = {};
						}
					}

					g.combo_end();
				}

				//curve type
				if (g.combo_begin("keyframe style"sv, to_string(_curve->frame_style)))
				{
					for (auto i = keyframe_style::begin; i < keyframe_style::end; i = next(i))
					{
						if (g.selectable(to_string(i), _curve->frame_style == i))
						{
							_curve->frame_style = i;
							mod = true;
						}
					}

					g.combo_end();
				}

				//sync
				if (g.checkbox("sync to clients"sv, _curve->sync))
					mod = true;

				//locked
				if (g.checkbox("lock in editor"sv, _curve->locked))
					mod = true;

				//hidden
				if (g.checkbox("hide in editor"sv, _curve->hidden))
					mod = true;

				// default value
				if(make_curve_default_value_editor(g, "default_value"sv, _curve,
					_curve->default_value, _curve_vec_edit_cache, _curve_edit_cache))
					mod = true;

				if (disabled)
					g.end_disabled();

				if (mod)
					std::invoke(generate_yaml, d);

				g.text("Resource as YAML:"sv);
				g.input_text_multiline("##yaml"sv, _yaml, {}, gui::input_text_flags::readonly);
			}
			g.window_end();
		}

		std::string resource_name() const override
		{
			return _title;
		}

		resources::resource_base* get() const noexcept override
		{
			return _base;
		}

	private:
		object_editor_ui<nullptr_t>::cache_map _curve_edit_cache;
		object_editor_ui<nullptr_t>::vector_curve_edit _curve_vec_edit_cache;
		string _title;
		string _yaml;
		resources::curve* _curve = {};
		resources::resource_base* _base = {};
	};

	class font_editor : public resource_editor
	{
	public:
		void set_target(data_manager& d, unique_id i, unique_id mod) override
		{
			_font = d.get<resources::font>(i, mod);
			_base = d.get_resource(i, mod);
			_source = _font->source.generic_string();
			_title = "font: "s + d.get_as_string(i) + "###font_win"s;

			_source_window = source_window{ &d, "Font source"s, _source,
				_base->loaded_archive_path.generic_string(),
				_base->loaded_path.generic_string(),
			"Font Files (*.ttf;*.otf;){.ttf,.otf}"sv, "Fonts"sv };

			return;
		}

		void update(data::data_manager& d, gui& g) override
		{
			const auto generate_yaml = [this](data::data_manager& d) {
				auto writer = data::make_writer();
				_base->serialise(d, *writer);
				_yaml = writer->get_string();
			};

			if (empty(_yaml))
				std::invoke(generate_yaml, d);

			g.next_window_size({}, gui::set_condition_enum::first_use);
			if (g.window_begin(_title))
			{
				const auto disabled = !editable(_base->mod);
				auto mod = false;
				if (disabled)
					g.begin_disabled();

				g.input("##font_source"sv, _source, gui::input_text_flags::readonly);
				g.same_line();
				if (g.button("Source..."))
					_source_window.open();

				if (disabled)
					g.end_disabled();

				if (mod)
					std::invoke(generate_yaml, d);

				g.text("Resource as YAML:"sv);
				g.input_text_multiline("##yaml"sv, _yaml, {}, gui::input_text_flags::readonly);
			}
			g.window_end();

			if (_source_window.update(g))
				_source = _source_window.get_new_source();

			return;
		}

		std::string resource_name() const override
		{
			return _title;
		}

		resources::resource_base* get() const noexcept override
		{
			return _base;
		}

	private:
		source_window _source_window;
		string _title;
		string _yaml;
		string _source;
		resources::font* _font = {};
		resources::resource_base* _base = {};
	};

	// game-system editor: not configurable with the current editor
	// level-scripts editor: not configurable with the current editor

	void add_or_update_curve_on_object(hades::data::data_manager& d, 
		hades::resources::object& o, const hades::resources::curve* c,
		const hades::resources::curve_default_value& value)
	{
		auto unloaded_iter = std::ranges::find(o.curves, c->id, [](auto&& unloaded_curve) {
			return unloaded_curve.curve_link.id();
			});

		if (end(o.curves) != unloaded_iter)
			unloaded_iter->value = to_string({ *c, value });
		else
		{
			o.curves.emplace_back(
				d.make_resource_link<hades::resources::curve>(c->id, o.id),
				to_string({ *c, value })
			);
		}

		return;
	}

	class object_editor : public resource_editor
	{
	public:
		void set_target(data_manager& d, unique_id i, unique_id mod) override
		{
			_obj = d.get<resources::object>(i, mod);
			_base = d.get_resource(i, mod);
			const auto str = d.get_as_string(i);
			_title = "object: "s + str + "###obj_win"s;
			
			_generate_combo(d, _base_objects, _obj->base, {}, "objects"sv);
			_base_objects.attached_ids.emplace_back(_obj->id);
			std::erase(_base_objects.combo, str);

			_generate_combo(d, _systems, _obj->systems, &_obj->all_systems, "systems"sv);
			_generate_combo(d, _render_systems, _obj->render_systems, &_obj->all_render_systems, "render systems"sv);

			_generate_curves_combo(d);

			const auto groups = d.get_all_names_for_type("animation group"sv);
			_anim_groups.clear();
			_anim_groups.reserve(size(groups));
			std::ranges::transform(groups, back_inserter(_anim_groups), [](auto&& str) {
				return to_string(str);
				});

			_tags.clear();
			_tags.reserve(size(_obj->tags));
			std::ranges::transform(_obj->tags, back_inserter(_tags), [&d](auto&& id) {
				return d.get_as_string(id);
				});

			const auto inherited_tags = resources::object_functions::get_inherited_tags(*_obj);
			_base_tags.clear();
			_base_tags.reserve(size(inherited_tags));
			std::ranges::transform(inherited_tags, back_inserter(_base_tags), [&d](auto&& pair)->std::pair<string, string> {
				return { d.get_as_string(pair.tag), d.get_as_string(pair.object) };
				});

			std::ranges::sort(_base_tags, {}, &std::pair<string, string>::first);

			/*for (const auto& s : _tags)
				erase(_base_tags, s);*/

			return;
		}

		void update(data::data_manager& d, gui& g) override
		{
			using hades::gui;

			const auto generate_yaml = [this](data::data_manager& d) {
				auto writer = data::make_writer();
				_base->serialise(d, *writer);
				_yaml = writer->get_string();
			};

			if (empty(_yaml))
				std::invoke(generate_yaml, d);

			g.next_window_size({}, gui::set_condition_enum::first_use);
			if (g.window_begin(_title))
			{
				const auto disabled = !editable(_base->mod);
				auto mod = false;
				if (disabled)
					g.begin_disabled();

				const auto add_base = [this](data::data_manager& d, unique_id id) {
					_obj->base.emplace_back(d.make_resource_link<resources::object>(id, _obj->id));
					return;
				};

				const auto rem_base = [this](data::data_manager& d, unique_id id) {
					const auto base = std::ranges::find(_obj->base, id);
					_obj->base.erase(base);
					return;
				};

				// base object list
				g.text("base objects"sv);
				if (_update_list_ui("base objects"sv, _base_objects, d, g, add_base, rem_base))
					mod = true;

				g.separator_horizontal();

				// animations
				const auto anim_groups_size = size(_anim_groups);
				const auto anim_preview = _anim_group_selected < anim_groups_size ?
					std::string_view{ _anim_groups[_anim_group_selected] } :
					"none"sv;
				if (g.combo_begin("animation group", anim_preview))
				{
					if (g.selectable("none"sv, !(_anim_group_selected < anim_groups_size)))
					{
						mod = true;
						_anim_group_selected = std::numeric_limits<std::size_t>::max();
						_obj->animations = {};
					}

					for (auto i = std::size_t{}; i < anim_groups_size; ++i)
					{
						if (g.selectable(_anim_groups[i], _anim_group_selected == i))
						{
							mod = true;
							_anim_group_selected = i;
							using namespace resources::animation_group_functions;
							_obj->animations = make_resource_link(d, d.get_uid(_anim_groups[i]), _obj->id);
						}
					}

					g.combo_end();
				}

				g.separator_horizontal();

				const auto curve_list_size = size(_add_curve);
				const auto prev = _curve_selected < curve_list_size ?
					std::string_view{ _add_curve[_curve_selected] } : "select a curve"sv;
				g.text("curves");
				if (g.combo_begin("##add_curve"sv, prev))
				{
					for (auto i = std::size_t{}; i < curve_list_size; ++i)
					{
						if (g.selectable(_add_curve[i], i == _curve_selected))
							_curve_selected = i;
					}
					g.combo_end();
				}

				g.same_line();
				if(g.button("add curve"sv) && _curve_selected < curve_list_size)
				{
					mod = true;
					const auto curve = d.get<resources::curve>(d.get_uid(_add_curve[_curve_selected]));
					add_or_update_curve_on_object(d, *_obj, curve, curve->default_value);
				}

				if (g.begin_table("curves_table"sv, 2, gui::table_flags::borders))
				{
					g.table_setup_column("###curve_col"sv, gui::table_column_flags::width_stretch, .8f);
					g.table_setup_column("###button_col"sv, gui::table_column_flags::width_stretch, .2f);

					for (auto& c : _obj->all_curves)
					{
						// curves inherited from base objects
						const auto foreign_curve = c.object != _obj->id;
						g.push_id(c.curve_ptr);
						if (g.table_next_column())
						{
							if (foreign_curve)
								g.begin_disabled();
							if (make_curve_default_value_editor(g, d.get_as_string(c.curve_ptr->id),
								c.curve_ptr, c.value, _curve_vec_edit_cache, _curve_edit_cache))
							{
								mod = true;
								add_or_update_curve_on_object(d, *_obj, c.curve_ptr, c.value);
							}
							if (foreign_curve)
								g.end_disabled();

							if (foreign_curve)
							{
								g.same_line();
								g.text_disabled(d.get_as_string(c.object));
							}
						}

						if (g.table_next_column())
						{
							if (foreign_curve)
							{
								if (g.button("override"sv))
								{
									mod = true;
									add_or_update_curve_on_object(d, *_obj, c.curve_ptr, c.value);
								}
							}
							else
							{
								if (g.button("remove"sv))
								{
									mod = true;
									auto unloaded_iter = std::ranges::find(_obj->curves, c.curve_ptr->id, [](auto&& unloaded_curve) {
										return unloaded_curve.curve_link.id();
										});

									assert(end(_obj->curves) != unloaded_iter);
									_obj->curves.erase(unloaded_iter);
								}
							}
						}
						g.pop_id();
					}

					g.end_table();
				}

				g.separator_horizontal();

				const auto add_system = [this](data::data_manager& d, unique_id id) {
					_obj->systems.emplace_back(d.make_resource_link<resources::system>(id, _obj->id));
					return;
				};

				const auto rem_system = [this](data::data_manager& d, unique_id id) {
					const auto iter = std::ranges::find(_obj->systems, id);
					_obj->systems.erase(iter);
					return;
				};

				// systems
				g.text("systems"sv);
				if (_update_list_ui("systems"sv, _systems, d, g, add_system, rem_system))
					mod = true;

				g.separator_horizontal();

				const auto add_rsystem = [this](data::data_manager& d, unique_id id) {
					_obj->render_systems.emplace_back(d.make_resource_link<resources::render_system>(id, _obj->id));
					return;
				};

				const auto rem_rsystem = [this](data::data_manager& d, unique_id id) {
					const auto iter = std::ranges::find(_obj->render_systems, id);
					_obj->render_systems.erase(iter);
					return;
				};

				// render systems
				g.text("render systems"sv);
				if (_update_list_ui("render systems"sv, _systems, d, g, add_rsystem, rem_rsystem))
					mod = true;

				g.separator_horizontal();

				// tags
				const auto tag_add = [&] {
					if (std::ranges::find(_tags, _next_tag) == end(_tags))
					{
						mod = true;
						_tags.emplace_back(_next_tag);
						std::ranges::sort(_tags);
						const auto id = d.get_uid(_next_tag);
						_obj->tags.emplace_back(id);
						_next_tag.clear();
					}

					return;
				};

				g.text("tags"sv);

				if (g.listbox_begin("inherited"sv))
				{
					for(const auto& [tag, source] : _base_tags)
					{ 
						g.selectable(tag, false);
						g.same_line();
						g.text_disabled(source);
					}

					g.listbox_end();
				}

				if (g.input_text("###next-tag"sv, _next_tag,
					gui::input_text_flags::enter_returns_true))
				{
					std::invoke(tag_add);
				}

				g.same_line();

				if (g.button("add tag"sv))
					std::invoke(tag_add);

				g.same_line();
				if (g.button("remove selected"sv) && _tag_selected < size(_tags))
				{
                    const auto iter = next(begin(_tags), signed_cast(_tag_selected));
					if (iter != end(_tags))
					{
						mod = true;
						const auto id = d.get_uid(*iter);
						_tags.erase(iter);
						erase(_obj->tags, id);
					}
				}

				g.listbox("tags"sv, _tag_selected, _tags);

				g.separator_horizontal();

				if (disabled)
					g.end_disabled();

				if (mod)
				{
					d.update_all_links();
					//reload to generate curve list
					_obj->load(d);
					_generate_curves_combo(d);
					std::invoke(generate_yaml, d);
				}

				g.text("Resource as YAML:"sv);
				g.input_text_multiline("##yaml"sv, _yaml, {}, gui::input_text_flags::readonly);
			}
			g.window_end();
			return;
		}

		std::string resource_name() const override
		{
			return _title;
		}

		resources::resource_base* get() const noexcept override
		{
			return _base;
		}

	private:
		//used for base objects,
		//	systems + render systems
		struct resource_list
		{
			std::vector<string> attached;
			std::vector<unique_id> attached_ids;  
			std::vector<string> combo;
			std::size_t attached_selected = std::numeric_limits<std::size_t>::max();
			std::size_t combo_selected = std::numeric_limits<std::size_t>::max();
		};

		void _generate_curves_combo(data::data_manager& d)
		{
			auto curves = d.get_all_names_for_type("curves"sv);
			const auto removed = std::ranges::remove_if(curves, [&d, &all_curves = _obj->all_curves](auto&& str)->bool {
				return std::ranges::find(all_curves, str, [&d](auto&& c)->std::string_view {
					return d.get_as_string(c.curve_ptr->id);
					}) != end(all_curves);
				});
			const auto unique = std::ranges::unique(begin(curves), begin(removed));

			_add_curve.clear();
			_add_curve.reserve(size(curves) - size(unique));
			std::transform(begin(curves), begin(unique), back_inserter(_add_curve), [](auto&& str) {
				return to_string(str); 
				});

			if (_curve_selected > size(_add_curve))
				_curve_selected = std::numeric_limits<std::size_t>::max();
		}

		template<typename T>
		void _generate_combo(data::data_manager& d, resource_list& r,
			const std::vector<resources::resource_link<T>>& source,
			const std::vector<resources::resource_link<T>>* all_source,
			std::string_view res_type)
		{
			const auto res_size = size(source);
			r.attached.reserve(res_size);
			r.attached_ids.reserve(res_size);
			for (const auto& rl : source)
			{
				const auto id = rl.id();
				r.attached.emplace_back(d.get_as_string(id));
				r.attached_ids.emplace_back(id);
			}

			std::ranges::sort(r.attached);

			auto resources = d.get_all_names_for_type(res_type);

			const auto remove_duplicates = [&](const auto& source) {
				const auto removed = std::ranges::remove_if(resources, [&d, &source](auto&& str)->bool {
					return std::ranges::find(source, str, [&d](auto&& c)->std::string_view {
						return d.get_as_string(c.id());
						}) != end(source);
					});
				const auto unique = std::ranges::unique(begin(resources), begin(removed));
				resources.erase(begin(unique), end(resources));
			};

			if (all_source)
				std::invoke(remove_duplicates, *all_source);
			else
				std::invoke(remove_duplicates, source);

			r.combo.clear();
			r.combo.reserve(size(resources));
			std::ranges::transform(resources, back_inserter(r.combo), [](auto&& str) {
				return to_string(str);
				});

			return;
		}

		template<typename AddFunc, typename RemoveFunc>
		bool _update_list_ui(std::string_view label, resource_list& res,
			data::data_manager& d, gui& g, AddFunc add, RemoveFunc remove)
		{
			auto ret = false;
			g.push_id(label);
			if (g.begin_table("##table"sv, 2))
			{
				if (g.table_next_column())
					g.listbox("##resource_list"sv, res.attached_selected, res.attached);

				if (g.table_next_column())
				{
					const auto size = std::size(res.combo);
					const auto combo_selected = res.combo_selected < size;
					const auto preview = combo_selected ?
						std::string_view{ res.combo[res.combo_selected] } :
						""sv;

					if (g.combo_begin("##add_combo"sv, preview))
					{
						for (auto i = std::size_t{}; i < size; ++i)
						{
							if (g.selectable(res.combo[i], i == res.combo_selected))
								res.combo_selected = i;
						}

						g.combo_end();
					}

					if (!combo_selected)
						g.begin_disabled();
					if (g.button("add"sv))
					{
						ret = true;
                        auto str_iter = next(begin(res.combo), signed_cast(res.combo_selected));
						const auto id = d.get_uid(*str_iter);
						res.attached_ids.emplace_back(id);
						res.attached.emplace_back(std::move(*str_iter));
						std::ranges::sort(res.attached);
						res.combo.erase(str_iter);
						std::invoke(add, d, id);
					}
					if (!combo_selected)
						g.end_disabled();

					const auto base_selected = res.attached_selected < std::size(res.attached);

					if (!base_selected)
						g.begin_disabled();

					if (g.button("remove"sv))
					{
						ret = true;
                        const auto iter = next(begin(res.attached), signed_cast(res.attached_selected));
						const auto id = d.get_uid(*iter);
						_base_objects.combo.emplace_back(std::move(*iter));
						std::ranges::sort(_base_objects.combo);

						res.attached.erase(iter);
						res.attached_selected = std::numeric_limits<std::size_t>::max();

						const auto id_iter = std::ranges::find(res.attached_ids, id);
						res.attached_ids.erase(id_iter);

						std::invoke(remove, d, id);
					}

					if (!base_selected)
						g.end_disabled();
				} 

				g.end_table();
			}

			g.pop_id();
			return ret;
		}

		object_editor_ui<nullptr_t>::cache_map _curve_edit_cache;
		object_editor_ui<nullptr_t>::vector_curve_edit _curve_vec_edit_cache;
		resource_list _base_objects;
		resource_list _systems;
		resource_list _render_systems;
		std::vector<string> _add_curve;
		std::vector<string> _anim_groups;
		std::vector<std::pair<string, string>> _base_tags;
		std::vector<string> _tags;
		std::size_t _curve_selected = std::numeric_limits<std::size_t>::max();
		std::size_t _anim_group_selected = std::numeric_limits<std::size_t>::max();
		std::size_t _tag_selected = std::numeric_limits<std::size_t>::max();
		string _title;
		string _yaml;
		string _next_tag;
		resources::object* _obj = {};
		resources::resource_base* _base = {};
	};

	void basic_resource_inspector::update(gui& g, data::data_manager& d)
	{
		_resource_tree(g, d);

		if (_tree_state.res_editor)
			_tree_state.res_editor->update(d, g);
		else
			_new_datafile = {};

		// TODO: func for this
		if (_new_datafile.open)
		{
			if (g.window_begin("create new data file"sv, _new_datafile.open))
			{
				g.text("Create a new datafile with the following name"sv);
				g.text("and move "sv);
				g.same_line();

				//remove hidden elements from the resource display name
				// res editors may be storing the imgui id in the end of the name
				// eg. "resource name##type"
				const auto name = _tree_state.res_editor->resource_name();
				constexpr auto hashes = std::array{ '#', '#' };
				const auto name_beg = begin(name);
				auto iter = std::find_first_of(name_beg, end(name), begin(hashes), end(hashes));
				g.text({ name_beg, iter });
				g.same_line();
				g.text(" into it."sv);

				g.input("filename"sv, _new_datafile.name);

				if (g.button("create"sv))
				{
					auto res = _tree_state.res_editor->get();
					res->data_file = _new_datafile.name;
					_refresh(d);
					_new_datafile = {};
				}

				if (g.button("cancel"sv))
				{
					_new_datafile = {};
				}
			}
			g.window_end();
		}
		return;
	}

	bool basic_resource_inspector::is_resource_open() const noexcept
	{
		return static_cast<bool>(_tree_state.res_editor);
	}

	void basic_resource_inspector::prompt_new_data_file() noexcept
	{
		assert(_tree_state.res_editor);
		_new_datafile.open = true;
		return;
	}

	const resources::resource_base* basic_resource_inspector::get_current_resource() const noexcept
	{
		if (!_tree_state.res_editor)
			return nullptr;
		return _tree_state.res_editor->get();
	}

	std::unique_ptr<resource_editor> basic_resource_inspector::make_resource_editor(std::string_view s)
	{
		if (s == "textures"sv)
			return std::make_unique<texture_editor>();
		else if (s == "animations"sv)
			return std::make_unique<animation_editor>();
		else if (s == "animation-groups"sv)
			return std::make_unique<animation_group_editor>();
		else if (s == "curves"sv)
			return std::make_unique<curve_editor>();
		else if (s == "fonts"sv)
			return std::make_unique<font_editor>();
		else if (s == "objects"sv)
			return std::make_unique<object_editor>();

		return {};
	}
	
	void basic_resource_inspector::_list_resources_from_data_file(
		resource_tree_state::group_iter first,
		resource_tree_state::group_iter last,
		gui& g,	data_manager& d, unique_id mod,
		vector_float& rect_max)
	{
		while (first != last)
		{
			auto type = first->res_type;
			auto next = std::next(first);
			while (next != last &&
				next->res_type == type)
				++next;

			const auto tree = g.tree_node(empty(type) ? "unknown"sv : type);
			const auto max = g.get_item_rect_max();
			rect_max.x = std::max(max.x, rect_max.x);
			rect_max.y = std::max(max.y, rect_max.y);

			if (tree)
			{
				_list_resources_from_group(first, next, g, d, mod, rect_max);
				g.tree_pop();
			}

			first = next;
		}
		return;
	}

	constexpr auto dragdrop_type_name = "resource_ptr"sv;

	void basic_resource_inspector::_list_resources_from_group(
		resource_tree_state::group_iter first,
		resource_tree_state::group_iter last,
		gui& g, data_manager& d, unique_id mod,
		vector_float& rect_max)
	{
		while (first != last)
		{
			auto tree_flags = gui::tree_node_flags::leaf;
			if (_tree_state.res_editor)
			{
				const auto res = _tree_state.res_editor->get();
				assert(res);
				if(res->mod == first->mod_id &&
					res->id == first->id)
					tree_flags |= gui::tree_node_flags::selected;
			}
			const auto tree_open = g.tree_node(first->name, tree_flags);
			const auto max = g.get_item_rect_max();
			rect_max.x = std::max(max.x, rect_max.x);
			rect_max.y = std::max(max.y, rect_max.y);

			if (_show_by_data_file && _mod == first->mod_id && g.begin_dragdrop_source(gui::dragdrop_flags::source_no_hold_to_open_others))
			{
				auto res = d.get_resource(first->id, mod);
				g.set_dragdrop_payload(res, dragdrop_type_name);
				g.text(first->name);
				g.end_dragdrop_source();
			}

			if (g.is_item_clicked() && !g.is_item_toggled_open())
			{
				_tree_state.res_editor = make_resource_editor(first->res_type);

				if (_tree_state.res_editor)
				{
					_tree_state.res_editor->set_target(d, first->id, mod);
					_tree_state.res_editor->set_editable_mod(_mod);
				}
			}

			if (mod != first->mod_id)
			{
				g.layout_horizontal();
				g.text_disabled(first->mod);
			}

			if (tree_open)
				g.tree_pop();

			++first;
		}
		return;
	}

	void basic_resource_inspector::_refresh(data::data_manager& d)
	{
		const auto dat = d.get_mod_stack();
		// build the list of resource types
		const auto& mod_index = _tree_state.mod_index;

		const auto stack_end = mod_index >= size(dat) ?
            end(dat) : next(begin(dat), signed_cast(mod_index + 1));
		
		auto& group = _tree_state.resource_groups;
		group.clear();

		auto iter = begin(dat);
		if (_show_by_data_file)
			std::advance(iter, mod_index);

        for (/*iter*/; iter != stack_end; ++iter)
		{
			const auto m = *iter;

			for (const auto& type : m->resources_by_type)
			{
				using val_type = resource_tree_state::group_val;
				std::transform(begin(type.second), end(type.second), back_inserter(group), [&type = type.first, &d, &mod = m->mod_info](auto&& elm)->val_type {
					return { type, elm->data_file.generic_string(), elm->id, d.get_as_string(elm->id), mod.id, mod.name };
				});
			}
		}

		const auto less = [](auto&& left, auto&& right) noexcept {
			return left.id < right.id;
		};

		const auto equal = [](auto&& left, auto&& right) noexcept {
			return left.id == right.id;
		};

		const auto beg = rbegin(group);
		const auto end = rend(group);
		std::stable_sort(beg, end, less);
		auto last = std::unique(beg, end, equal);
		group.erase(end.base(), last.base());

		if (_show_by_data_file)
		{
			std::ranges::sort(group, [](auto&& left, auto&& right) {
					return std::tie(left.data_file, left.res_type, left.name) <
						std::tie(right.data_file, right.res_type, right.name);
				});
		}
		else
		{
			std::ranges::sort(group, [](auto&& left, auto&& right) {
					return std::tie(left.res_type, left.name) <
						std::tie(right.res_type, right.name);
				});
		}

		return;
	}

	void basic_resource_inspector::_resource_tree(gui& g, data::data_manager& d)
	{
		const auto mods = d.get_mod_stack();
		// main resource list
		// need to set min size manually
		// since the child creates a scrolling region that can fit in a tiny window
		g.next_window_size({}, hades::gui::set_condition_enum::first_use);
		if (g.window_begin("resource tree"sv, gui::window_flags::no_collapse))
		{
			if (_tree_state.mod_index >= size(mods))
			{
				assert(size(mods) > 0);
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

			const auto no_mod = size(mods) == 1;
			if (no_mod)
				g.begin_disabled();

			if (g.combo_begin("Mod"sv, preview_str))
			{
				// list other mod options
				for (auto i = std::size_t{}; i < size(mods); ++i)
				{
					auto& mod_info = mods[i]->mod_info;
					auto str = mod_info.name; // copy
					if (mod_info.id == _mod)
						str += "*"s;

					if (g.selectable(str, i == current_mod))
						_tree_state.mod_index = i;
				}

				g.combo_end();
			}

			const auto built_in = _tree_state.mod_index == std::size_t{};
			if (built_in)
				g.begin_disabled();
			const auto data_file_toggle = g.checkbox("Show by data file"sv, _show_by_data_file);
			if (built_in)
				g.end_disabled();
			if (no_mod)
				g.end_disabled();

			if (current_mod != _tree_state.mod_index ||
				data_file_toggle)
			{
				if (built_in)
					_show_by_data_file = false;
				_refresh(d);
			}

			g.next_window_size({}, hades::gui::set_condition_enum::first_use);
			if (g.child_window_begin("##resource-tree-list"sv))
			{
				auto iter = begin(_tree_state.resource_groups);
				const auto end = std::end(_tree_state.resource_groups);

				if (_show_by_data_file)
				{	
					auto needs_refresh = false;
					while (iter != end)
					{
						const auto& data_file = iter->data_file;
						auto next = std::next(iter);
						while (next != end && next->data_file == data_file)
							++next;
						
						assert(!empty(data_file));
						const auto tree_node = g.tree_node(data_file);
						const auto min = g.get_item_rect_min();
						auto max = g.get_item_rect_max();

						if (tree_node)
						{
							_list_resources_from_data_file(iter, next, g, d,
								mods[_tree_state.mod_index]->mod_info.id, max);	
							g.tree_pop();
						}

						constexpr auto min_offset = 4.f;
						if (ImGui::BeginDragDropTargetCustom(
							ImRect{ min.x + min_offset, min.y + min_offset, max.x, max.y },
							ImGui::GetID((data_file + "+dragdrop_target"s).c_str())))
						{
							const auto drop = g.accept_dragdrop_payload<resources::resource_base*>(dragdrop_type_name);
							if (drop.is_delivery())
							{
								assert(drop.is_correct_type(dragdrop_type_name));
								auto ret = drop.get();
								ret->data_file = data_file;
								needs_refresh = true;
							}
							g.end_dragdrop_target();
						}

						iter = next;
					}

					if (needs_refresh)
						_refresh(d);
				}
				else
				{
					auto unused_max = gui::vector2{};
					_list_resources_from_data_file(iter, end, g, d,
							mods[_tree_state.mod_index]->mod_info.id, unused_max);
				} // ! _show_by_data_file
			} g.child_window_end();
		}
		g.window_end();
	}
}
