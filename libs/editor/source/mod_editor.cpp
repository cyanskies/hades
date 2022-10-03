#include "hades/mod_editor.hpp"

#include <iterator>

#include "hades/colour.hpp"
#include "hades/console_variables.hpp"
#include "hades/properties.hpp"
#include "hades/standard_paths.hpp"

using namespace std::string_view_literals;

namespace hades
{
	void mod_editor_impl::init()
	{
		reinit();
		return;
	}
	
	bool mod_editor_impl::handle_event(const event& e)
	{
		_gui.activate_context();
		return _gui.handle_event(e);
	}

	static bool mod_exists(std::string_view name) noexcept
	{
		try
		{
			auto root = std::filesystem::path{ "."sv };

			if (!console::get_bool(cvars::file_portable, cvars::default_value::file_portable))
				root = user_custom_file_directory();

			auto filename = std::filesystem::path{ name };
			auto deflate = console::get_bool(cvars::file_deflate, cvars::default_value::file_deflate);
			if (filename.has_extension())
				return std::filesystem::exists(root / filename);

			filename.replace_extension(zip::resource_archive_ext());
			if (std::filesystem::exists(root / filename))
				return true;

			filename.replace_extension();
			return std::filesystem::is_directory(root / filename);
		}
		catch (const std::exception& exc)
		{
			log_warning(exc.what());
			return true;
		}
		catch (...)
		{
			log_error("Unexpected error"sv);
			return true;
		}
	}

	void mod_editor_impl::update(time_duration dt, const sf::RenderTarget&, input_system::action_set)
	{
		_gui.activate_context();
		_gui.update(dt);
		_gui.frame_begin();

		[[maybe_unused]] auto [data_man, lock] = data::detail::get_data_manager_exclusive_lock();
		
		if (_gui.main_menubar_begin())
		{
			if (_gui.menu_begin("File"sv))
			{
				if (_gui.menu_item("New mod..."sv, !_current_mod))
				{
					_new_mod = {};
					_new_mod.exists = mod_exists(_new_mod.name);
					_new_mod.open = true;
				}
				if (_gui.menu_begin("Edit mod"sv, !_current_mod))
				{
					if(_gui.menu_item("Load mod..."sv))
						_load_mod.open = true;

					for (const auto& m : _mods)
					{
						const auto disabled = data::data_manager::built_in_mod_name() == m;
						if (disabled)
							_gui.begin_disabled();
						if (_gui.menu_item(m, !_current_mod))
						{
							_mode = edit_mode::already_loaded;
							_set_loaded_mod(m, *data_man);
						}
						if (disabled)
							_gui.end_disabled();
					}

					_gui.menu_end();
				}

				if (_gui.menu_item("Save mod"sv, _current_mod))
					data_man->export_mod(_current_mod);

				if (_gui.menu_item("Save mod as..."sv, _current_mod))
				{
					const auto& name = data_man->get_as_string(_current_mod);
					_save_as_window = { name, mod_exists(name), true };
				}

				if (_gui.menu_item("Close mod"sv, _current_mod))
					_close_mod(*data_man);

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
					_gui.text_coloured("mod with this name already exists"sv, colours::red);
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
					_set_mod(_load_mod.name, *data_man);
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
					_save_as_window.exists = mod_exists(_save_as_window.name);
				const auto disabled = _save_as_window.name == data::data_manager::built_in_mod_name();
				if (disabled)
					_gui.begin_disabled();

				if (_gui.button("Save"sv))
					data_man->export_mod(_current_mod, _save_as_window.name);
				
				if (disabled)
					_gui.end_disabled();

				if (_save_as_window.exists)
				{
					_gui.same_line();
					_gui.text_coloured("A mod with this name already exists, overwrite?"sv, colours::red);
				}
			}
			_gui.window_end();
		}

		_mod_properties(_gui, *data_man);

		_inspector.update(_gui, *data_man);
		_gui.frame_end();
	}

	void mod_editor_impl::draw(sf::RenderTarget& r, time_duration)
	{
		r.setView(_gui_view);
		r.draw(_backdrop);
		_gui.activate_context();
		r.draw(_gui);
	}
	
	void mod_editor_impl::reinit()
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

		auto [data_man, lock] = data::detail::get_data_manager_exclusive_lock();

		const auto mods = data_man->get_mod_stack();
		_mods.clear();

		for (const auto& m : mods)
			_mods.emplace_back(m->mod_info.name);
		return;
	}

	void mod_editor_impl::_close_mod(data::data_manager& d)
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

		_current_mod = unique_zero;

		return;
	}

	void mod_editor_impl::_save_mod(data::data_manager& d)
	{
		//if (/*portable*/)
		//	;

		//if (/*compressed*/)
		//	;
	}

	void mod_editor_impl::_mod_properties(gui& g, data::data_manager& d)
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
						iter = next(iter, _edit_mod_window.dep_selected);
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

				//deps
				//includes
				//pretty name
			}
			g.window_end();
		}		
	}

	void mod_editor_impl::_set_loaded_mod(std::string_view mod, data::data_manager& d)
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

	void mod_editor_impl::_set_mod(std::string_view mod, data::data_manager& d)
	{
		_mod_count = d.get_mod_count();
		if (d.try_load_mod(mod))
		{
			_set_loaded_mod(mod, d);
		}

		return;
	}
}
