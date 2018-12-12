#include "hades/level_editor.hpp"

#include "hades/camera.hpp"
#include "hades/files.hpp"
#include "hades/level.hpp"
#include "hades/level_editor_component.hpp"
#include "hades/properties.hpp"
#include "hades/mouse_input.hpp"

namespace hades::detail
{
	level_editor_impl::level_editor_impl()
	{
		const auto default_size = console::get_int(cvars::editor_level_default_size);

		_level.map_x = *default_size;
		_level.map_y = *default_size;
		using namespace std::string_literals;
		_level.name = editor::new_level_name;
		_level.description = editor::new_level_description;
	}

	level_editor_impl::level_editor_impl(level l) : _level(std::move(l))
	{
	}

	void level_editor_impl::init()
	{
		_camera_height = console::get_int(cvars::editor_camera_height_px);
		_toolbox_width = console::get_int(cvars::editor_toolbox_width);
		_toolbox_auto_width = console::get_int(cvars::editor_toolbox_auto_width);

		_scroll_margin = console::get_int(cvars::editor_scroll_margin_size);
		_scroll_rate = console::get_float(cvars::editor_scroll_rate);

		_new_level_options.width = _level.map_x;
		_new_level_options.height = _level.map_y;

		//set world camera to 0, 0
		_world_view.setCenter({});

		_handle_component_setup();

		_load(_level);

		reinit();
	}

	bool level_editor_impl::handle_event(const event &e)
	{
		return _gui.handle_event(e);
	}

	static void clamp_camera(sf::View &camera, vector_float min, vector_float max)
	{
		const auto pos = camera.getCenter();

		const auto new_x = std::clamp(pos.x, min.x, max.x);
		const auto new_y = std::clamp(pos.y, min.y, max.y);

		camera.setCenter({ new_x, new_y });
	}

	void level_editor_impl::reinit()
	{
		const auto width = console::get_int(cvars::video_width);
		const auto height = console::get_int(cvars::video_height);
		const auto view_height = console::get_int(cvars::editor_camera_height_px);

		_window_width = static_cast<float>(*width);
		_window_height = static_cast<float>(*height);

		_gui_view.reset({ 0.f, 0.f, _window_width, _window_height });

		camera::variable_width(_world_view, static_cast<float>(*view_height), _window_height, _window_width);
		clamp_camera(_world_view, { 0.f, 0.f }, { static_cast<float>(_level.map_x), static_cast<float>(_level.map_y) });
		_gui.set_display_size({ _window_width, _window_height });

		_background.setSize({ _window_width, _window_height });
		const auto background_colour = sf::Color{200u, 200u, 200u, 255u};
		_background.setFillColor(background_colour);
	}

	void level_editor_impl::update(time_duration dt, const sf::RenderTarget &t, input_system::action_set actions)
	{
		_total_run_time += dt;

		const auto mouse_position = actions.find(input::mouse_position);
		assert(mouse_position != std::end(actions));

		//view scrolling
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

				clamp_camera(_world_view, { 0.f, 0.f }, { static_cast<float>(_level.map_x), static_cast<float>(_level.map_y) });
			}

			if (_active_brush != invalid_brush)
			{
				//generate draw preview
				const auto world_coords = mouse::to_world_coords(t, { mouse_position->x_axis, mouse_position->y_axis }, _world_view);
				_generate_brush_preview(_active_brush, dt, world_coords);
			}
		}

		const auto world_mouse_pos = mouse::to_world_coords(t, { mouse_position->x_axis, mouse_position->y_axis }, _world_view);

		const auto mouse_left = actions.find(input::mouse_left);
		assert(mouse_left != std::end(actions));
		mouse::update_button_state(*mouse_left, *mouse_position, _total_run_time, _mouse_left);

		if (mouse::is_click(_mouse_left))
			_component_on_click(_active_brush, world_mouse_pos);
		else if (mouse::is_drag_start(_mouse_left))
			_component_on_drag_start(_active_brush, world_mouse_pos);
		else if (mouse::is_dragging(_mouse_left))
			_component_on_drag(_active_brush, world_mouse_pos);
		else if (mouse::is_drag_end(_mouse_left))
			_component_on_drag_end(_active_brush, world_mouse_pos);

		_update_gui(dt);
	}

	void level_editor_impl::draw(sf::RenderTarget &rt, time_duration dt)
	{
		rt.setView(_gui_view);
		rt.draw(_background);

		rt.setView(_world_view);
		_draw_components(rt, dt, _active_brush);

		rt.setView(_gui_view);
		rt.draw(_gui);
	}

	void level_editor_impl::_set_active_brush(std::size_t index)
	{
		_active_brush = index;
	}

	void level_editor_impl::_load(const level &l)
	{
		_level_x = l.map_x;
		_level_y = l.map_y;

		_component_on_load(l);

		_level = l;

		reinit();
	}

	void level_editor_impl::_save()
	{
		const auto path = _save_path;
		level new_level{};
		new_level.map_x = _level_x;
		new_level.map_y = _level_y;

		new_level = _component_on_save(new_level);

		const auto save = serialise(new_level);

		try
		{
			files::write_file(_next_save_path, save);
			_level = std::move(new_level);
			_save_path = _next_save_path;
		}
		catch (const files::file_exception &e)
		{
			LOGERROR(e.what());
		}
	}

	void level_editor_impl::_update_gui(time_duration dt)
	{
		_gui.update(dt);
		_gui.frame_begin();

		using namespace std::string_view_literals;

		//make main menu
		const auto main_menu_created = _gui.main_menubar_begin();
		assert(main_menu_created);

		if (_gui.menu_begin("file"sv))
		{
			if (_gui.menu_item("new..."sv))
				_window_flags.new_level = true;
			if(_gui.menu_item("load..."sv))
				_window_flags.load_level = true;
			_gui.menu_item("save"sv);
			if (_gui.menu_item("save as..."sv))
				_window_flags.save_level = true;
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
				_gui.button("Create"sv);
				_gui.layout_horizontal();
				if (_gui.button("Cancel"))
					_window_flags.new_level = false;

				_gui.text("Level Size: "sv);
				_gui.input("Width"sv, _new_level_options.width);
				_gui.input("Height"sv, _new_level_options.height);
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
					
					const auto file = [this] {
						if (_load_level_mod.empty())
							return files::read_file(_load_level_path);
						else
							return files::as_string(_load_level_mod, _load_level_path);
					}();

					if (!file.empty())
					{
						const auto l = deserialise(file);
						_load(l);
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
				if (_gui.button("Save"))
				{
					_window_flags.save_level = false;
					_save();
				}

				_gui.input_text("path", _next_save_path);
			}
			_gui.window_end();
		}

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

	console::create_property(cvars::editor_level_default_size, cvars::default_value::editor_level_default_size);
}
