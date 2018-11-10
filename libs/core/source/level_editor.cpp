#include "hades/level_editor.hpp"

#include "hades/level_editor_component.hpp"
#include "hades/properties.hpp"
#include "hades/mouse_input.hpp"

namespace hades::detail
{
	void level_editor_impl::init()
	{
		_camera_height = console::get_int(cvars::editor_camera_height_px);
		_toolbox_width = console::get_int(cvars::editor_toolbox_width);
		_toolbox_auto_width = console::get_int(cvars::editor_toolbox_auto_width);

		_scroll_margin = console::get_int(cvars::editor_scroll_margin_size);
		_scroll_rate = console::get_float(cvars::editor_scroll_rate);

		_handle_component_setup();

		reinit();
	}

	bool level_editor_impl::handle_event(const event &e)
	{
		return _gui.handle_event(e);
	}

	void level_editor_impl::reinit()
	{
		const auto width = console::get_int(cvars::video_width);
		const auto height = console::get_int(cvars::video_height);

		_window_width = static_cast<float>(*width);
		_window_height = static_cast<float>(*height);

		_gui_view.reset({ 0.f, 0.f, _window_width, _window_height });

		const float screen_ratio = _window_width / _window_height;
		const auto camera_height = static_cast<float>(*_camera_height);
		//set size to preserve view position
		_world_view.setSize({ camera_height * screen_ratio, camera_height });

		_gui.set_display_size({ _window_width, _window_height });
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
				else if (mouse_position->y_axis > static_cast<int32>(_window_height) + margin)
					_world_view.move({ 0.f, rate });

				const auto pos = _world_view.getCenter();

				const auto new_x = std::clamp(pos.x, 0.f, static_cast<float>(_level_x));
				const auto new_y = std::clamp(pos.y, 0.f, static_cast<float>(_level_y));

				_world_view.setCenter({ new_x, new_y });
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
		rt.setView(_world_view);
		_draw_components(rt, dt, _active_brush);

		rt.setView(_gui_view);
		rt.draw(_gui);
	}

	void level_editor_impl::_set_active_brush(std::size_t index)
	{
		_active_brush = index;
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
			_gui.menu_item("new..."sv);
			_gui.menu_item("load..."sv);
			_gui.menu_item("save"sv);
			_gui.menu_item("save as..."sv);
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

		//make infobox
		//make minimap

		_update_component_gui(_gui);

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
