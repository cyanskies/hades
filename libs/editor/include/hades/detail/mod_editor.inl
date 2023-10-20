#include "hades/mod_editor.hpp"

#include <iterator>

#include "hades/animation.hpp"
#include "hades/colour.hpp"
#include "hades/console_variables.hpp"
#include "hades/curve_extra.hpp"
#include "hades/font.hpp"
#include "hades/objects.hpp"
#include "hades/properties.hpp"
#include "hades/standard_paths.hpp"
#include "hades/texture.hpp"

using namespace std::string_view_literals;

namespace hades
{
	template<debug::is_resource_inspector ResourceInspector>
	inline mod_editor_impl<ResourceInspector>::mod_editor_impl()
	{
		using namespace std::string_literals;

		using dm = data::data_manager;
		using rb = resources::resource_base*;

		// create funcs
		auto texture_func = [](dm& d, unique_id n, unique_id m)->rb {
			auto tex = resources::texture_functions::find_create_texture(d, n, m);
			assert(tex);
			return resources::texture_functions::get_resource_base(*tex);
		};

		auto anim_func = [](dm& d, unique_id n, unique_id m)->rb {
			auto anim = resources::animation_functions::find_or_create(d, n, m);
			assert(anim);
			return resources::animation_functions::get_resource_base(*anim);
		};

		auto anim_group_func = [](dm& d, unique_id n, unique_id m)->rb {
			auto anim = resources::animation_group_functions::find_or_create(d, n, m);
			assert(anim);
			return resources::animation_group_functions::get_resource_base(*anim);
		};

		auto curve_func = [](dm& d, unique_id n, unique_id m)->rb {
			auto curve = d.find_or_create<resources::curve>(n, m, "curves"sv);
			resources::make_curve_valid(*curve);
			return curve;
		};

		auto font_func = [](dm& d, unique_id n, unique_id m)->rb {
			return d.find_or_create<resources::font>(n, m, "fonts"sv);
		};

		auto obj_func = [](dm& d, unique_id n, unique_id m)->rb {
			return d.find_or_create<resources::object>(n, m, "objects"sv);
		};

		_create_resource_funcs = unordered_map_string<create_resource_function>{
			{ "texture"s, texture_func },
			{ "animation"s, anim_func },
			{ "animation group"s, anim_group_func },
			{ "curve"s, curve_func },
			{ "font"s, font_func },
			{ "object"s, obj_func }
		};
	}

	template<debug::is_resource_inspector ResourceInspector>
	void mod_editor_impl<ResourceInspector>::init()
	{
		reinit();
		return;
	}

	template<debug::is_resource_inspector ResourceInspector>
	bool mod_editor_impl<ResourceInspector>::handle_event(const event& e)
	{
		_gui.activate_context();
		return _gui.handle_event(e);
	}

	namespace detail
	{
		bool mod_exists(std::string_view name) noexcept;
	}

	template<debug::is_resource_inspector ResourceInspector>
	void mod_editor_impl<ResourceInspector>::update(time_duration dt, const sf::RenderTarget&, input_system::action_set)
	{
		_gui.activate_context();
		_gui.update(dt);
		_gui.frame_begin();

		auto& data_man = data::detail::get_data_manager();

		if (_gui.main_menubar_begin())
		{
			if (_gui.menu_begin("File"sv))
			{
				if (_gui.menu_item("New mod..."sv, !_current_mod))
				{
					_new_mod = {};
					_new_mod.exists = detail::mod_exists(_new_mod.name);
					_new_mod.open = true;
				}
				if (_gui.menu_begin("Edit mod"sv, !_current_mod))
				{
					if (_gui.menu_item("Load mod..."sv))
						_load_mod.open = true;

					for (const auto& m : _mods)
					{
						const auto disabled = data::data_manager::built_in_mod_name() == m;
						if (disabled)
							_gui.begin_disabled();
						if (_gui.menu_item(m, !_current_mod))
						{
							_close_windows();
							_mode = edit_mode::already_loaded;
							_set_loaded_mod(m, data_man);
						}
						if (disabled)
							_gui.end_disabled();
					}

					_gui.menu_end();
				}

				if (_gui.menu_item("Save mod"sv, _current_mod))
					data_man.export_mod(_current_mod);

				if (_gui.menu_item("Save mod as..."sv, _current_mod))
				{
					_close_windows();
					const auto& name = data_man.get_as_string(_current_mod);
					_save_as_window = { name, detail::mod_exists(name), true };
				}

				if (_gui.menu_item("Close mod"sv, _current_mod))
					_close_mod(data_man);

				if (_gui.menu_item("Exit"sv))
					kill();

				_gui.menu_end();
			}

			if (_gui.menu_begin("Mod"sv))
			{
				if (_gui.menu_item("Edit mod properties...", _current_mod))
				{
					_edit_mod_window.open = true;
				}
				_gui.menu_end();
			}

			if (_gui.menu_begin("Resource"sv))
			{
				if (_gui.menu_item("Create new resource..."sv, _current_mod))
				{
					_new_res_window = new_resource_window{};
					_new_res_window.open = true;
				}

				const auto res_open = _inspector.is_resource_open();
				const auto res = _inspector.get_current_resource();
				if (_gui.menu_item("Create new datafile for current resource"sv, res_open && res->mod == _current_mod))
				{
					_inspector.prompt_new_data_file();
				}
				_gui.menu_end();
			}

			_gui.main_menubar_end();
		}

		if (_new_mod.open)
		{
			_gui.next_window_size({}, gui::set_condition_enum::first_use);
			if (_gui.window_begin("New mod"sv, _new_mod.open))
			{
				_gui.input("Mod filename"sv, _new_mod.name);
				const auto disabled = _new_mod.name == data::data_manager::built_in_mod_name();
				if (disabled)
					_gui.begin_disabled();
				if (_gui.button("Create"sv))
				{
					_mode = edit_mode::normal;
					log_debug("Created new mod: " + _new_mod.name);
				}
				if (disabled)
					_gui.end_disabled();

				if (_new_mod.exists)
				{
					_gui.same_line();
					_gui.text_coloured("mod with this name already exists"sv, colours::from_name(colours::names::red));
				}
			}
			_gui.window_end();
		}

		if (_load_mod.open)
		{
			_gui.next_window_size({}, gui::set_condition_enum::first_use);
			if (_gui.window_begin("Load mod"sv, _load_mod.open))
			{
				_gui.input("Mod filename"sv, _load_mod.name);
				const auto disabled = _load_mod.name == data::data_manager::built_in_mod_name();
				if (disabled)
					_gui.begin_disabled();
				if (_gui.button("Load"sv))
				{
					_mode = edit_mode::normal;
					_set_mod(_load_mod.name, data_man);
					log_debug("Loaded mod: " + _load_mod.name);
				}
				if (disabled)
					_gui.end_disabled();
			}
			_gui.window_end();
		}

		if (_save_as_window.open)
		{
			_gui.next_window_size({}, gui::set_condition_enum::first_use);
			if (_gui.window_begin("Save mod"sv, _save_as_window.open))
			{
				if (_gui.input("Mod filename"sv, _save_as_window.name))
					_save_as_window.exists = detail::mod_exists(_save_as_window.name);
				const auto disabled = _save_as_window.name == data::data_manager::built_in_mod_name();
				if (disabled)
					_gui.begin_disabled();

				if (_gui.button("Save"sv))
					data_man.export_mod(_current_mod, _save_as_window.name);

				if (disabled)
					_gui.end_disabled();

				if (_save_as_window.exists)
				{
					_gui.same_line();
					_gui.text_coloured("A mod with this name already exists, overwrite?"sv, colours::from_name(colours::names::red));
				}
			}
			_gui.window_end();
		}

		if (_new_res_window.open)
		{
			_create_new_resource(_gui, data_man);
		}

		_mod_properties(_gui, data_man);

		_inspector.update(_gui, data_man);
		_gui.frame_end();
	}

	template<debug::is_resource_inspector ResourceInspector>
	void mod_editor_impl<ResourceInspector>::draw(sf::RenderTarget& r, time_duration)
	{
		r.setView(_gui_view);
		r.draw(_backdrop);
		_gui.activate_context();
		r.draw(_gui);
	}

	template<debug::is_resource_inspector ResourceInspector>
	void mod_editor_impl<ResourceInspector>::reinit()
	{
		const auto width = console::get_int(cvars::video_width, cvars::default_value::video_width);
		const auto height = console::get_int(cvars::video_height, cvars::default_value::video_height);

		const auto fwidth = static_cast<float>(width->load());
		const auto fheight = static_cast<float>(height->load());

		_gui_view.reset({ {}, { fwidth, fheight } });
		_gui.activate_context();
		_gui.set_display_size({ fwidth, fheight });

		_backdrop.setPosition({ 0.f, 0.f });
		_backdrop.setSize({ fwidth, fheight });
		const auto background_colour = sf::Color{ 200u, 200u, 200u, 255u };
		_backdrop.setFillColor(background_colour);

		auto& data_man = data::detail::get_data_manager();

		const auto mods = data_man.get_mod_stack();
		_mods.clear();

		for (const auto& m : mods)
			_mods.emplace_back(m->mod_info.name);
		return;
	}

	template<debug::is_resource_inspector ResourceInspector>
	inline void mod_editor_impl<ResourceInspector>::register_resource_type(std::string type, create_resource_function func)
	{
		_create_resource_funcs.emplace(std::move(type), std::move(func));
		return;
	}

	template<debug::is_resource_inspector ResourceInspector>
	void mod_editor_impl<ResourceInspector>::_close_mod(data::data_manager& d)
	{
		switch (_mode)
		{
		case edit_mode::normal:
		{
			const auto mod_count = d.get_mod_count();
			const auto count_diff = mod_count - _mod_count;
			for (auto i = std::size_t{}; i < count_diff; ++i)
				d.pop_mod();
		}
		case edit_mode::already_loaded:
		{
			// do nothing
		}
		}

		_close_windows();

		_current_mod = unique_zero;
		return;
	}

	template<debug::is_resource_inspector ResourceInspector>
	void mod_editor_impl<ResourceInspector>::_close_windows()
	{
		_edit_mod_window = {};
		_load_mod = {};
		_new_mod = {};
		_save_as_window = {};
		return;
	}

	template<debug::is_resource_inspector ResourceInspector>
	inline void mod_editor_impl<ResourceInspector>::_create_new_resource(gui& g, data::data_manager& d)
	{
		if (g.window_begin("create new resource"sv))
		{
			if (g.input("name"sv, _new_res_window.name))
			{
				const auto id = d.get_uid(_new_res_window.name);
				const auto res = d.try_get_resource(id, _current_mod);
				_new_res_window.exists = res != nullptr;
			}

			const auto begin = std::begin(_create_resource_funcs);
			auto cur_type = next(begin, _new_res_window.type_index);
			assert(cur_type != end(_create_resource_funcs));
			if (g.combo_begin("resource type"sv, cur_type->first))
			{
				auto iter = begin;
				const auto end = std::end(_create_resource_funcs);
				while(iter != end)
				{
					if (g.selectable(iter->first, iter == cur_type))
						_new_res_window.type_index = integer_cast<std::size_t>(distance(begin, iter));

					++iter;
				}

				g.combo_end();
			}

			if (_new_res_window.exists)
			{
				g.text_coloured("Resource name is already used"sv, colours::from_name(colours::names::red));
				g.begin_disabled();
			}

			if (g.button("create"sv))
			{
				const auto id = d.get_uid(_new_res_window.name);
				auto new_res = std::invoke(cur_type->second, d, id, _current_mod);
				_inspector.refresh(d);
				_inspector.inspect(d, new_res->id, new_res->mod);
				_new_res_window.open = false;
			}

			if (_new_res_window.exists)
				g.end_disabled();
		}
		g.window_end();
		return;
	}

	template<debug::is_resource_inspector ResourceInspector>
	void mod_editor_impl<ResourceInspector>::_save_mod(data::data_manager&)
	{
		//if (/*portable*/)
		//	;

		//if (/*compressed*/)
		//	;
	}

	template<debug::is_resource_inspector ResourceInspector>
	void mod_editor_impl<ResourceInspector>::_mod_properties(gui& g, data::data_manager& d)
	{
		if (_edit_mod_window.open)
		{
			g.next_window_size({}, gui::set_condition_enum::first_use);
			if (g.window_begin("Edit mod properties"sv, _edit_mod_window.open))
			{
				g.input("Display name"sv, _edit_mod_window.pretty_name);
				if (g.begin_table("##property_table"sv, 2))
				{
					g.table_next_column();
					// dependency list
					g.text("Dependencies"sv);

					const auto disabled = size(_edit_mod_window.dependencies) <= _edit_mod_window.dep_selected;
					if (disabled)
						g.begin_disabled();
					if (g.button("Remove selected"sv))
					{
						auto iter = begin(_edit_mod_window.dependencies);
						iter = next(iter, signed_cast(_edit_mod_window.dep_selected));
						_edit_mod_window.dependencies.erase(iter);

						if (_edit_mod_window.dep_selected > 0)
							--_edit_mod_window.dep_selected;
					}
					if (disabled)
						g.end_disabled();

					g.listbox("##dep_listbox"sv, _edit_mod_window.dep_selected,
						_edit_mod_window.dependencies, std::mem_fn(&std::pair<string, unique_id>::first));

					g.table_next_column();

					// current mods
					g.text("Loaded mods"sv);

					// returns true if the mod is in the dependency list
					const auto already_depened = [this](const auto& mod) noexcept {
						return std::any_of(begin(_edit_mod_window.dependencies), end(_edit_mod_window.dependencies),
							[id = mod->mod_info.id](const auto& dep) {
								return dep.second == id;
							});
					};

					const auto mod_stack = d.get_mod_stack();
					auto add_disabled = _edit_mod_window.mods_selected >
						integer_cast<std::size_t>(std::count_if(begin(mod_stack),
							end(mod_stack), std::not_fn(already_depened)));

					const auto stack_end = size(mod_stack);
					assert(stack_end > 0);
					data::data_manager::resource_storage* selected_mod = {};
					for (auto i = std::size_t{}, j = std::size_t{}; i < stack_end; ++i)
					{
						const auto& m = mod_stack[i];
						if (!std::invoke(already_depened, m))
						{
							const auto k = i - j;
							if (k == _edit_mod_window.mods_selected)
							{
								selected_mod = m;
								break;
							}
						}
						else
							++j;
					}
					assert(selected_mod);

					if (selected_mod->mod_info.id == _current_mod ||
						d.is_built_in_mod(selected_mod->mod_info.id))
						add_disabled = true;

					if (add_disabled)
						g.begin_disabled();
					if (g.button("Add selected"sv))
					{
						_edit_mod_window.dependencies.emplace_back(selected_mod->mod_info.name, selected_mod->mod_info.id);

						if (_edit_mod_window.mods_selected > 0)
							--_edit_mod_window.mods_selected;
					}
					if (add_disabled)
						g.end_disabled();

					if (g.listbox_begin("##loaded_mods"sv))
					{
						for (auto i = std::size_t{}, j = std::size_t{}; i < stack_end; ++i)
						{
							const auto& m = mod_stack[i];
							const auto elm_disabled = m->mod_info.id == _current_mod ||
								d.is_built_in_mod(m->mod_info.id);

							if (elm_disabled)
								g.begin_disabled();
							if (!std::invoke(already_depened, m))
							{
								const auto k = i - j;
								if (g.selectable(m->mod_info.name, _edit_mod_window.mods_selected == k))
									_edit_mod_window.mods_selected = k;
							}
							else
								++j;
							if (elm_disabled)
								g.end_disabled();
						}
						g.listbox_end();
					}
					g.end_table();
					g.text("Dependencies won't be loaded unless this mod is closed and reloaded"sv);
				}

				//includes
				//pretty name
			}
			g.window_end();
		}
	}

	template<debug::is_resource_inspector ResourceInspector>
	void mod_editor_impl<ResourceInspector>::_set_loaded_mod(std::string_view mod, data::data_manager& d)
	{
		const auto uid = d.get_uid(mod);
		assert(!d.is_built_in_mod(uid));
		_inspector.lock_editing_to(uid);
		_current_mod = uid;
		_new_mod = {};
		_load_mod = {};

		const auto& m = d.get_mod(uid);

		_edit_mod_window = {};
		_edit_mod_window.dependencies.reserve(size(m.dependencies));
		std::transform(begin(m.dependencies), end(m.dependencies), back_inserter(_edit_mod_window.dependencies),
			[&d](const auto& uid) {
				return std::make_pair(d.get_as_string(uid), uid);
			});

		_edit_mod_window.pretty_name = m.name;

		return;
	}

	template<debug::is_resource_inspector ResourceInspector>
	void mod_editor_impl<ResourceInspector>::_set_mod(std::string_view mod, data::data_manager& d)
	{
		_mod_count = d.get_mod_count();
		if (d.try_load_mod(mod))
		{
			_set_loaded_mod(mod, d);
		}

		return;
	}
}
