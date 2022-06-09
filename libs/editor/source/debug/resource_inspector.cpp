#include "hades/debug/resource_inspector.hpp"

#include "hades/animation.hpp"
#include "hades/data.hpp"
#include "hades/gui.hpp"
#include "hades/texture.hpp"
#include "hades/utility.hpp"
#include "hades/writer.hpp"

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace hades::data
{
	static std::vector<std::pair<string, make_resource_editor_fn>> resource_editors;

	void register_resource_editor(std::string_view s, make_resource_editor_fn fn)
	{
		auto end = std::end(resource_editors);
		auto iter = std::find_if(begin(resource_editors), end, [s](auto&& elm) {
			return elm.first == s;
			});

		if (iter != end)
			iter->second = std::move(fn);
		else
			resource_editors.emplace_back(s, std::move(fn));
		return;
	}

	class animation_editor : public resource_editor
	{
	public:
		void set_target(data_manager& d, unique_id i, unique_id mod) override
		{
			namespace anim = resources::animation_functions;
			_anim = anim::get_resource(d, i, mod);
			_name = "Animation: "s + d.get_as_string(i) + "###anim-edit"s;
			const auto texture = anim::get_texture(*_anim);
			const auto tex_id = resources::texture_functions::get_id(texture);
			_tex_name = d.get_as_string(tex_id);
			_duration = anim::get_duration(*_anim);
			_base = d.get_resource(i);

			// size of first frame
			_anim_frames = anim::get_animation_frames(*_anim);
			assert(!empty(_anim_frames));
			const auto& first = _anim_frames.front();
			_size = { abs(first.w * first.scale_w), abs(first.h * first.scale_h) };
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

			if (g.window_begin(_name, gui::window_flags::no_collapse))
			{
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

				g.text("Resource as YAML:"sv);
				g.input_text_multiline("##yaml"sv, _yaml, {}, gui::input_text_flags::readonly);
			}

			g.window_end();

			if (g.window_begin("Animation frames"sv, gui::window_flags::no_collapse))
			{
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

						if (g.input_scalar("Relative Duration"sv, iter->duration))
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
			}
			g.window_end();

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
				
				g.checkbox("Play Animation"sv, _play);
				if (_play)
					g.begin_disabled();
				g.slider_int("Frame"s, _current_frame, 0, size(_anim_frames) - 1, gui::slider_flags::no_input);
				if (_play)
					g.end_disabled();
				if (g.child_window_begin("##anim-preview"sv, {}, false, gui::window_flags::horizontal_scrollbar))
				{
					const auto float_size = gui::vector2{ _size[0], _size[1] };
					if (_play)
					{
						const auto& io = ImGui::GetIO();
						_time += io.DeltaTime * _play_speed;
						const auto sec = seconds_float{ _time };
						const auto time = time_point{ time_cast<time_duration>(sec) };
						g.image(*_anim, float_size * _scale, time);

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
						assert(integer_cast<std::size_t>(_current_frame) < size(_anim_frames));
						const auto iter = next(begin(_anim_frames), _current_frame);
						_time = std::transform_reduce(begin(_anim_frames), iter,
							0.f, std::plus<>(), std::mem_fn(&resources::animation_frame::normalised_duration));
						_time *= time_cast<seconds_float>(_duration).count(); // -> to seconds first



						const auto sec = seconds_float{ _time };
						const auto time = time_cast<time_duration>(sec);// *_duration.count();
						g.image(*_anim, float_size* _scale, time_point{ time });
					}
					
				}
				g.child_window_end();
			}

			g.window_end();
			return;
		}

	private:
		resources::animation* _anim;
		resources::resource_base* _base;
		time_duration _duration;
		int _current_frame = int{};
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
			_texture_base = d.get_resource(i, mod); // TODO: pass mod here
			_texture = tex::get_resource(d, i);
			_name = "Texture: "s + d.get_as_string(i) + "###texture-edit"s;
			_smooth = tex::get_smooth(_texture);
			_mips = tex::get_mips(_texture);
			_repeat = tex::get_repeat(_texture);
			_source = tex::get_source(_texture);
			const auto size = tex::get_size(_texture);
			_size[0] = size.x;
			_size[1] = size.y;
		}

		void update(data::data_manager& d,gui& g) override
		{
			const auto generate_yaml = [this](data::data_manager& d) {
				auto writer = data::make_writer();
				_texture_base->serialise(d, *writer);
				_yaml = writer->get_string();
			};

			if (empty(_yaml))
				generate_yaml(d);

			if (g.window_begin(_name, gui::window_flags::no_collapse))
			{
				auto mod = false;
				mod |= g.checkbox("Smooth"sv, _smooth)
					| g.checkbox("Repeating"sv, _repeat)
					| g.checkbox("Mip Maping"sv, _mips)
					| g.input_scalar("Size"sv, _size);

				g.begin_disabled();
				g.input_text("Source"sv, _source);
				g.end_disabled();

				if (mod)
				{
					resources::texture_functions::set_settings(_texture, { _size[0], _size[1] },
						_smooth, _repeat, _mips, true);
					generate_yaml(d);
				}

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
					g.image(*_texture, { {}, float_size }, float_size * _scale);
				}
				g.child_window_end();
			}
			
			g.window_end();
			return;
		}

	private:
		const resources::resource_base* _texture_base;
		resources::texture* _texture;
		string _name;
		string _yaml;
		string _source;
		bool _smooth;
		bool _mips;
		bool _repeat;
		float _scale = 1.f;
		std::array<texture_size_t, 2> _size;
	};

	static std::unique_ptr<resource_editor> make_resource_editor(std::string_view s)
	{
		auto end = std::end(resource_editors);
		auto iter = std::find_if(begin(resource_editors), end, [s](auto&& elm) {
			return elm.first == s;
			});

		if (iter != end)
			return std::invoke(iter->second);

		if (s == "textures"sv)
			return std::make_unique<texture_editor>();
		else if (s == "animations"sv)
			return std::make_unique<animation_editor>();

		return {};
	}

	void resource_inspector::update(gui& g, data::data_manager* d)
	{
		assert(d);
		_resource_tree(g, d);

		if (_tree_state.res_editor)
			_tree_state.res_editor->update(*d, g);
		return;
	}

	void resource_inspector::_list_resources_from_group(resource_inspector::resource_tree_state::group& group, gui& g,
		data::data_manager& d, unique_id mod)
	{
		auto ids = std::vector<unique_id>{};

		for (auto& res : group.resources)
		{
			if (g.tree_node(d.get_as_string(res), gui::tree_node_flags::leaf))
			{
				if (g.is_item_clicked() && !g.is_item_toggled_open())
				{
					_tree_state.res_editor = make_resource_editor(group.name);
					if(_tree_state.res_editor)
						_tree_state.res_editor->set_target(d, res, mod);
				}

				g.tree_pop();
			}
		}
	}

	void resource_inspector::_refresh(data::data_manager& d)
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
				auto group_iter = std::find_if(begin(g), end(g), [&](auto&& elm) {
					return elm.name == type.first;
				});

				if (group_iter == end(g))
					group = &g.emplace_back(resource_tree_state::group{ type.first });
				else
					group = &*group_iter;

				std::transform(begin(type.second), end(type.second), back_inserter(group->resources), [](auto&& elm) {
					return elm->id;
				});
			}
		}

		for (auto&& group : _tree_state.resource_groups)
			remove_duplicates(group.resources);

		return;
	}

	void resource_inspector::_resource_tree(gui& g, data::data_manager* d)
	{
		const auto mods = d->get_mod_stack();
		//main resource list
		if (g.window_begin("resource tree"sv, gui::window_flags::no_collapse))
		{
			if (_tree_state.mod_index >= size(mods))
			{
				_tree_state.mod_index = size(mods) - 1;
				_refresh(*d);
			}

			auto current_mod = _tree_state.mod_index;
			if (g.combo_begin("Mod"sv, mods[current_mod]->mod_info.name))
			{
				//list other mod options
				for (auto i = std::size_t{}; i < size(mods); ++i)
				{
					if (g.selectable(mods[i]->mod_info.name, i == current_mod))
						_tree_state.mod_index = i;
				}

				g.combo_end();
			}

			if (current_mod != _tree_state.mod_index)
				_refresh(*d);

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
							_list_resources_from_group(resource_type, g, *d,
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