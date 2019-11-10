#include "hades/mission_editor.hpp"

#include "hades/level_editor.hpp"

namespace hades
{
	void create_mission_editor_console_variables()
	{
		create_level_editor_console_vars();
		return;
	}

	void register_mission_editor_resources(data::data_manager& d)
	{
		register_level_editor_resources(d);
		return;
	}

	void mission_editor_t::init()
	{
		const auto mis = new_mission();

		//create an empty mission
		_mission_desc = mis.description;
		_mission_name = mis.name;
		_mission_src = "new_mission.mission";

		reinit();
		return;
	}

	bool mission_editor_t::handle_event(const event& e)
	{
		_gui.activate_context();
		return _gui.handle_event(e);
	}

	void mission_editor_t::update(time_duration dt, const sf::RenderTarget& t, input_system::action_set a)
	{
		_editor_time += dt;
		_update_gui(dt);
	}

	void mission_editor_t::draw(sf::RenderTarget &t, time_duration dt)
	{
		t.setView(_gui_view);
		t.draw(_backdrop);
		_gui.activate_context();
		t.resetGLStates();
		t.draw(_gui);
	}

	void mission_editor_t::reinit()
	{
		const auto width = console::get_int(cvars::video_width, cvars::default_value::video_width);
		const auto height = console::get_int(cvars::video_height, cvars::default_value::video_height);

		const auto fwidth = static_cast<float32>(width->load());
		const auto fheight = static_cast<float32>(height->load());

		_gui_view.reset({ 0.f, 0.f, fwidth, fheight });
		_gui.activate_context();
		_gui.set_display_size({ fwidth, fheight });

		_backdrop.setPosition({0.f, 0.f});
		_backdrop.setSize({ fwidth, fheight });
		const auto background_colour = sf::Color{ 200u, 200u, 200u, 255u };
		_backdrop.setFillColor(background_colour);
	}

	void mission_editor_t::resume()
	{
	}

	mission mission_editor_t::new_mission()
	{
		//create an empty mission
		auto mis = mission{};
		mis.description = "An empty mission";
		mis.name = "Empty mission";

		//no starting ents or levels

		return mis;
	}

	void mission_editor_t::_gui_menu_bar()
	{
		if (_gui.menu_begin("editor"))
		{
			_gui.menu_item("new...", false);
			_gui.menu_item("load...", false);
			_gui.menu_item("save", false);
			_gui.menu_item("save as...", false);
			_gui.menu_end();
		}

		return;
	}

	void mission_editor_t::_gui_level_window()
	{
		return;
	}

	void mission_editor_t::_gui_object_window()
	{
		return;
	}

	void mission_editor_t::_update_gui(time_duration dt)
	{
		_gui.activate_context();
		_gui.update(dt);
		_gui.frame_begin();
		//_gui.show_demo_window();

		if (_gui.main_menubar_begin())
		{
			_gui_menu_bar();
			_gui.main_menubar_end();
		}

		if (_gui.window_begin("Levels", _level_w))
			_gui_level_window();
		_gui.window_end();
		
		if (_gui.window_begin("Objects", _obj_w))
			_gui_object_window();
		_gui.window_end();

		_gui.frame_end();
	}
}
