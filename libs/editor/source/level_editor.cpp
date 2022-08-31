#include "hades/level_editor.hpp"

#include "hades/camera.hpp"
#include "hades/files.hpp"
#include "hades/level.hpp"
#include "hades/level_editor_component.hpp"
#include "hades/level_editor_grid.hpp"
#include "hades/level_editor_level_properties.hpp"
#include "hades/level_editor_objects.hpp"
#include "hades/parser.hpp"
#include "hades/properties.hpp"
#include "hades/mouse_input.hpp"
#include "hades/mission_editor.hpp"

namespace hades::detail
{
	level_editor_impl::level_editor_impl()
		: _level_ptr{std::make_unique<level>()}
	{}

	level_editor_impl::level_editor_impl(const mission_editor_t* m, level* l)
		: _mission_editor{m}, _level{l}
	{}

	void level_editor_impl::init()
	{
		_camera_height = console::get_int(cvars::editor_camera_height_px,
			cvars::default_value::editor_camera_height_px);
		_toolbox_width = console::get_int(cvars::editor_toolbox_width,
			cvars::default_value::editor_toolbox_width);
		_toolbox_auto_width = console::get_int(cvars::editor_toolbox_auto_width,
			cvars::default_value::editor_toolbox_auto_width);

		_scroll_margin = console::get_int(cvars::editor_scroll_margin_size, 
			cvars::default_value::editor_scroll_margin_size);
		_scroll_rate = console::get_float(cvars::editor_scroll_rate, 
			cvars::default_value::editor_scroll_rate);

		_force_whole_tiles = console::get_int(cvars::editor_level_force_whole_tiles,
			cvars::default_value::editor_level_force_whole_tiles);

		const auto default_size = console::get_int(cvars::editor_level_default_size,
			cvars::default_value::editor_level_default_size);

		const auto size = default_size->load();
		_new_level_options.width = size;
		_new_level_options.height = size;

		//set world camera to 0, 0
		_world_view.setCenter({});

		_handle_component_setup();
		if (_mission_mode())
		{
			if (_level->map_x == 0 ||
				_level->map_y == 0)
			{
				_level->map_x = size;
				_level->map_y = size;
				*_level = _component_on_new(std::move(*_level));
			}
		}
		else
		{
			_level = _level_ptr.get();
			_level->map_x = size;
			_level->map_y = size;
			*_level = _component_on_new(std::move(*_level));
		}

		_load(_level);
		reinit();
	}

	bool level_editor_impl::handle_event(const event &e)
	{
		_gui.activate_context();
		return _gui.handle_event(e);
	}

	static void clamp_camera(sf::View &camera, vector_float min, vector_float max)
	{
		const auto& pos = camera.getCenter();

		const auto new_x = std::clamp(pos.x, min.x, max.x);
		const auto new_y = std::clamp(pos.y, min.y, max.y);

		camera.setCenter({ new_x, new_y });
	}

	void level_editor_impl::reinit()
	{
		const auto width = console::get_int(cvars::video_width,
			cvars::default_value::video_width);
		const auto height = console::get_int(cvars::video_height,
			cvars::default_value::video_height);
		const auto view_height = console::get_int(cvars::editor_camera_height_px,
			cvars::default_value::editor_camera_height_px);

		_window_width = static_cast<float>(*width);
		_window_height = static_cast<float>(*height);

		_new_level_options.width = _level->map_x;
		_new_level_options.height = _level->map_y;

		_gui.activate_context();
		_gui_view.reset({ {}, { _window_width, _window_height } });

		camera::variable_width(_world_view, static_cast<float>(*view_height), _window_width, _window_height);
		clamp_camera(_world_view, { 0.f, 0.f }, { static_cast<float>(_level->map_x), static_cast<float>(_level->map_y) });
		_gui.set_display_size({ _window_width, _window_height });
		_background.setSize({ _window_width, _window_height });
		const auto background_colour = sf::Color{200u, 200u, 200u, 255u};
		_background.setFillColor(background_colour);
	}

	level level_editor_impl::get_level() const
	{
		return *_level;
	}

	void level_editor_impl::update(time_duration dt, const sf::RenderTarget &t, input_system::action_set actions)
	{
		_total_run_time += dt;

		const auto mouse_position = actions.find(action{ input::mouse_position });
		assert(mouse_position != std::end(actions));

		if (mouse_position->active)
		{
			//world scroll
			//skip if under ui
			const auto under_ui = mouse_position->x_axis < _left_min
				|| mouse_position->y_axis < _top_min;

			if (!under_ui)
			{
				const auto rate = static_cast<float>(*_scroll_rate);
				const int32 margin = *_scroll_margin;
				if (mouse_position->x_axis < margin + _left_min)
					_world_view.move({ -rate, 0.f });
				else if (mouse_position->x_axis > static_cast<int32>(_window_width) - margin)
					_world_view.move({ rate, 0.f });

				if (mouse_position->y_axis < margin + _top_min)
					_world_view.move({ 0.f, -rate });
				else if (mouse_position->y_axis > static_cast<int32>(_window_height) - margin)
					_world_view.move({ 0.f, rate });

				clamp_camera(_world_view, { 0.f, 0.f }, { static_cast<float>(_level->map_x), static_cast<float>(_level->map_y) });
			}

			const auto world_mouse_pos = mouse::to_world_coords(t, { mouse_position->x_axis, mouse_position->y_axis }, _world_view);

			if (_active_brush != invalid_brush)
				_generate_brush_preview(_active_brush, dt, world_mouse_pos);

			const auto mouse_left = actions.find(action{ input::mouse_left });
			assert(mouse_left != std::end(actions));
			mouse::update_button_state(*mouse_left, *mouse_position, _total_run_time, _mouse_left);

			if (mouse::is_click(_mouse_left))
				_component_on_click(_active_brush, world_mouse_pos);
			else if (mouse::is_drag_start(_mouse_left))
				_component_on_drag_start(_active_brush, mouse::to_world_coords(t, _mouse_left.click_pos, _world_view));
			else if (mouse::is_dragging(_mouse_left))
				_component_on_drag(_active_brush, world_mouse_pos);
			else if (mouse::is_drag_end(_mouse_left))
				_component_on_drag_end(_active_brush, world_mouse_pos);
		}

		_update_gui(dt);
	}

	void level_editor_impl::draw(sf::RenderTarget &rt, time_duration dt)
	{
		rt.setView(_gui_view);
		rt.draw(_background);

		rt.setView(_world_view);
		_gui.activate_context();
		_draw_components(rt, dt, _active_brush);

		rt.setView(_gui_view);
		rt.draw(_gui);
	}

	level_editor_impl::get_players_return_type level_editor_impl::get_players() const
	{
		return _mission_editor->get_players();
	}

	void level_editor_impl::_set_active_brush(std::size_t index) noexcept
	{
		_active_brush = index;
	}

	bool level_editor_impl::_mission_mode() const
	{
		return _mission_editor;
	}

	void level_editor_impl::_load(level *l)
	{
		_level_x = l->map_x;
		_level_y = l->map_y;

		_component_on_load(*l);

		_level = l;

		reinit();
	}

	void level_editor_impl::_save()
	{
		level new_level{};
		new_level.map_x = _level_x;
		new_level.map_y = _level_y;

		new_level = _component_on_save(new_level);

		const auto save = serialise(new_level);

		try
		{
			if(!_mission_mode())
				files::write_file(_next_save_path, save);
			*_level = std::move(new_level);
			_save_path = _next_save_path;
		}
		catch (const files::file_error &e)
		{
			LOGERROR(e.what());
		}
	}

	void level_editor_impl::_update_gui(time_duration dt)
	{
		_gui.activate_context();
		_gui.update(dt);
		_gui.frame_begin();

		using namespace std::string_view_literals;
		using namespace std::string_literals;

		constexpr auto error_str = "error"sv;

		//FIXME: remove this bool, just call open_model directly
		if (_error_modal)
			_gui.open_modal(error_str);

		if (_gui.modal_begin(error_str))
		{
			_gui.text(_error_msg);

			if (_gui.button("ok"sv))
			{
				_error_modal = false;
				_gui.close_current_modal();
			}

			_gui.modal_end();
		}

		//make main menu
		const auto main_menu_created = _gui.main_menubar_begin();
		assert(main_menu_created);

		if (_gui.menu_begin("level editor"sv))
		{
			if (_gui.menu_item(_mission_mode() ? "reset to..."sv : "new..."sv))
				_window_flags.new_level = true;
			if(_gui.menu_item(_mission_mode() ? "replace from file..."sv : "load..."sv))
				_window_flags.load_level = true;

			if (_gui.menu_item("resize..."sv))
			{
				_resize_options.size = { _level_x, _level_y };
				_resize_options.offset = {};
				_resize_options.top_left = {};
				_resize_options.bottom_right = {};
				_window_flags.resize_level = true;
			}

			if (_gui.menu_item("save"sv))
				_save();
			if (_gui.menu_item("save as..."sv, !_mission_mode()))
				_window_flags.save_level = true;
			if (_mission_mode())
			{
				if (_gui.menu_item("return to mission editor"sv))
				{
					_save();
					state::kill();
				}
			}
			else
			{
				if (_gui.menu_item("exit editor"sv))
					state::kill();
			}
			
			_gui.menu_end();
		}

		_gui.main_menubar_end();

		//make toolbar
		const auto toolbar_created = _gui.main_toolbar_begin();
		assert(toolbar_created);
		const auto toolbar_y2 = _gui.window_position().y + _gui.window_size().y;
		_top_min = static_cast<int32>(toolbar_y2);
		_gui.main_toolbar_end();

		//make toolbox window
		assert(_toolbox_width);
		assert(_toolbox_auto_width);

		_gui.next_window_position({ 0.f, toolbar_y2 });
		const auto toolbox_size = [](int32 width, int32 auto_width, float window_width)->float {
			constexpr auto auto_mode = cvars::default_value::editor_toolbox_width;
			if (width == auto_mode)
				return window_width / auto_width;
			else
				return static_cast<float>(width);
		}(*_toolbox_width, *_toolbox_auto_width, _window_width);

		_gui.next_window_size({ toolbox_size, _window_height - toolbar_y2 });
		const auto toolbox_created = _gui.window_begin(editor::gui_names::toolbox, gui::window_flags::panel);
		assert(toolbox_created);
		//store toolbox x2 for use in the input update
		_left_min = static_cast<int32>(_gui.get_item_rect_max().x);
		_gui.window_end();

		//windows
		if (_window_flags.new_level)
		{
			if (_gui.window_begin(editor::gui_names::new_level, _window_flags.new_level))
			{
				if (_gui.button("Create"sv))
				{
					level l;
					l.map_x = _new_level_options.width;
					l.map_y = _new_level_options.height;

					try 
					{
						if (_mission_mode())
						{
							auto lev = _component_on_new(l);
							*_level = std::move(lev);
							_load(_level);
						}
						else
						{
							_level_ptr = std::make_unique<level>(_component_on_new(l));
							_load(_level_ptr.get());
						}

						_window_flags.new_level = false;
					}
					catch (const new_level_editor_error &e)
					{
						_error_modal = true;
						_error_msg = e.what();
					}
				}

				_gui.layout_horizontal();
				if (_gui.button("Cancel"sv))
					_window_flags.new_level = false;

				auto width = _new_level_options.width;
				auto height = _new_level_options.height;

				const auto force = _force_whole_tiles->load();

				if (force == 1)
				{
					width = _new_level_options.width / _tile_settings->tile_size;
					height = _new_level_options.height / _tile_settings->tile_size;
					_gui.text("Level Size(tiles): "sv);
				}
				else if (force > 1)
				{
					width = _new_level_options.width / (_tile_settings->tile_size * force);
					height = _new_level_options.height / (_tile_settings->tile_size * force);
					_gui.text("Level Size(tiles * "s + to_string(force) + "): "s);
				}
				else
					_gui.text("Level Size(pixels): "sv);

				_gui.input("Width"sv, width);
				_gui.input("Height"sv, height);

				if (force > 0)
				{
					_new_level_options.width = width * _tile_settings->tile_size * force;
					_new_level_options.height = height * _tile_settings->tile_size * force;
				}
				else
				{
					_new_level_options.width = width;
					_new_level_options.height = height;
				}
			}
			_gui.window_end();
		}

		if (_window_flags.resize_level)
		{
			if (_gui.window_begin(editor::gui_names::resize_level, _window_flags.resize_level))
			{
				if (_resize_options.offset.x < 0 || _resize_options.offset.y < 0 ||
					_resize_options.offset.x + _level_x > _resize_options.size.x ||
					_resize_options.offset.y + _level_y > _resize_options.size.y)
					_gui.text_coloured("current offset or new size will cause some sections of the current map to be erased"sv, sf::Color::Red);
				else
					_gui.text_coloured("no problems"sv, sf::Color::Green);

				if (_gui.button("Resize"sv))
				{	
					_component_on_resize(_resize_options.size, _resize_options.offset);
					_level_x = _resize_options.size.x;
					_level_y = _resize_options.size.y;

					reinit();
					_window_flags.resize_level = false;
				}

				_gui.text("current size(pixels): " + to_string(_level_x) + ", " + to_string(_level_y));

				bool changed = false;
				auto new_size = std::array{ _resize_options.size.x, _resize_options.size.y };

				const auto force = _force_whole_tiles->load();

				if (force > 0)
				{
					const auto s = _resize_options.size / (integer_cast<int32>(_tile_settings->tile_size) * force);
					new_size = std::array{ s.x, s.y };
				}

				if (_gui.input("new size"sv, new_size))
					changed = true;

				if (force > 0)
				{
					_resize_options.size = { 
						new_size[0] * _tile_settings->tile_size * force,
						new_size[1] * _tile_settings->tile_size * force 
					};
				}
				else
					_resize_options.size = { new_size[0], new_size[1] };

				auto new_offset = std::array{ _resize_options.offset.x, _resize_options.offset.y };

				if (force > 0)
				{
					const auto o = _resize_options.offset / (integer_cast<int32>(_tile_settings->tile_size) * force);
					new_offset = std::array{ o.x, o.y };
				}

				if (_gui.input("current map offset"sv, new_offset))
					changed = true;

				if (force > 0 )
				{
					_resize_options.offset = { 
						new_offset[0] * _tile_settings->tile_size * force,
						new_offset[1] * _tile_settings->tile_size * force
					};
				}
				else
					_resize_options.offset = { new_offset[0], new_offset[1] };

				if (changed)
				{
					_resize_options.top_left = _resize_options.offset;
					_resize_options.bottom_right = _resize_options.size - 
						(_resize_options.offset + vector_int{_level_x, _level_y});
					changed = false;
				}

				auto tl = _resize_options.top_left;
				auto br = _resize_options.bottom_right;
				if (force > 0)
				{
					tl = _resize_options.top_left / (integer_cast<int32>(_tile_settings->tile_size) * force);
					br = _resize_options.bottom_right / (integer_cast<int32>(_tile_settings->tile_size) * force);
				}

				if (_gui.input("expand left by"sv, tl.x))
					changed = true;
				if(_gui.input("expand top by"sv, tl.y))
					changed = true;
				if(_gui.input("expand bottom by"sv, br.y))
					changed = true;
				if(_gui.input("expand right by"sv, br.x))
					changed = true;

				if (force > 0) 
				{
					_resize_options.top_left = tl * integer_cast<int32>(_tile_settings->tile_size) * force;
					_resize_options.bottom_right = br * integer_cast<int32>(_tile_settings->tile_size) * force;
				}

				if (changed)
				{
					_resize_options.offset = _resize_options.top_left;
					_resize_options.size = _resize_options.bottom_right + 
						vector_int{ _level_x, _level_y } + _resize_options.offset;
				}
			}
			_gui.window_end();
		}

		if (_window_flags.load_level)
		{
			if (_gui.window_begin(editor::gui_names::load_level, _window_flags.load_level))
			{
				if (_gui.button("Load"sv))
				{
					_window_flags.load_level = false;
					
					// TODO: use streams here
					const auto file = [this]()->std::unique_ptr<std::istream> {
						if (_load_level_mod.empty())
							return std::make_unique<files::ifstream>(files::stream_file(_load_level_path));
						else
						{
							const auto mod_id = data::get_uid(_load_level_mod);
							const auto& mod = data::get_mod(mod_id);
							return std::make_unique<irfstream>(files::stream_resource(mod, _load_level_path));
						}
					}();

					if (file->good())
					{
						auto parser = data::make_parser(*file);
						if (_mission_mode())
						{
							auto l = deserialise_level(*parser);
							*_level = std::move(l);
							_load(_level);
						}
						else
						{
							_level_ptr = std::make_unique<level>(deserialise_level(*parser));
							_load(_level_ptr.get());
						}
					}
				}
				_gui.text("leave mod empty to ignore"sv);
				_gui.input_text("mod"sv, _load_level_mod);
				_gui.input_text("path"sv, _load_level_path);
			}
			_gui.window_end();
		}

		if (_window_flags.save_level)
		{
			if (_gui.window_begin(editor::gui_names::save_level, _window_flags.save_level))
			{
				if (_gui.button("Save"sv))
				{
					_window_flags.save_level = false;
					_save();
				}

				_gui.input_text("path"sv, _next_save_path);
			}
			_gui.window_end();
		}

		//TODO:
		//make infobox
		//make minimap

		_update_component_gui(_gui, _window_flags);

		_gui.frame_end();
	}
}

void hades::create_editor_console_variables()
{
	console::create_property(cvars::editor_toolbox_width, cvars::default_value::editor_toolbox_width);
	console::create_property(cvars::editor_toolbox_auto_width, cvars::default_value::editor_toolbox_auto_width);

	console::create_property(cvars::editor_scroll_margin_size, cvars::default_value::editor_scroll_margin_size);
	console::create_property(cvars::editor_scroll_rate, cvars::default_value::editor_scroll_rate);

	console::create_property(cvars::editor_camera_height_px, cvars::default_value::editor_camera_height_px);

	console::create_property(cvars::editor_zoom_max, cvars::default_value::editor_zoom_max);
	console::create_property(cvars::editor_zoom_default, cvars::default_value::editor_zoom_default);

	console::create_property(cvars::editor_level_force_whole_tiles, cvars::default_value::editor_level_force_whole_tiles);
	console::create_property(cvars::editor_level_default_size, cvars::default_value::editor_level_default_size);
}

void hades::register_level_editor_resources(data::data_manager &d)
{
	register_background_resource(d); //used by level_editor_level_properties
	register_level_editor_object_resources(d);
	register_level_editor_terrain_resources(d);
}

void hades::create_level_editor_console_vars()
{
	create_editor_console_variables();
	create_level_editor_grid_variables();
	create_level_editor_properties_variables();
	create_level_editor_regions_variables();
	create_level_editor_terrain_variables();
	return;
}
