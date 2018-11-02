#include "hades/level_editor.hpp"

#include "hades/properties.hpp"
//TODO: common_input.hpp

namespace hades::detail
{
	void level_editor_impl::init()
	{
		_camera_height = console::get_int(cvars::editor_camera_height_px);
		_toolbox_width = console::get_int(cvars::editor_toolbox_width);
		_toolbox_auto_width = console::get_int(cvars::editor_toolbox_auto_width);

		_hand_component_setup();

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

	void level_editor_impl::update(time_duration dt, const sf::RenderTarget &, input_system::action_set actions)
	{
		//const auto mouse_position = actions.find()
	}

	void level_editor_impl::draw(sf::RenderTarget &rt, time_duration dt)
	{
		rt.setView(_world_view);
		_draw_components(rt, dt);

		rt.setView(_gui_view);
		rt.draw(_gui);
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
