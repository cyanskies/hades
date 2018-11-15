#include "hades/level_editor_level_properties.hpp"

#include "hades/gui.hpp"

namespace hades
{
	static void make_level_detail_window(gui &g, bool &open, string &name, string &description)
	{
		using namespace std::string_view_literals;
		if (g.window_begin("level details"sv, open))
		{
			if (g.button("Done"sv))
				open = false;

			g.input_text("Name"sv, name, gui::input_text_flags::auto_select_all);
			g.input_text_multiline("Description"sv, description, vector_float{}, gui::input_text_flags::no_horizontal_scroll);
		}
		g.window_end();
	}

	void level_editor_level_props::level_load(const level &l)
	{
		_new_name = editor::new_level_name;
		_new_desc = editor::new_level_description;

		_level_name = l.name;
		_level_desc = l.description;

		_background.setSize({ static_cast<float>(l.map_x),
							  static_cast<float>(l.map_y) });

		//TODO: background colour
		// paralax background etc...
		_background.setFillColor(sf::Color::Black);
	}

	void level_editor_level_props::gui_update(gui &g, editor_windows &flags)
	{
		using namespace std::string_view_literals;
		g.main_menubar_begin();
		if (g.menu_begin("level"sv))
		{
			if (g.menu_item("details..."sv))
				_details_window = true;
				
			g.menu_item("background..."sv);

			g.menu_end();
		}

		g.main_menubar_end();

		if (_details_window)
			make_level_detail_window(g, _details_window, _level_name, _level_desc);

		if (flags.new_level)
		{
			if (g.window_begin(editor::gui_names::new_level, flags.new_level))
			{
				g.input_text("Name", _new_name, gui::input_text_flags::auto_select_all);
				g.input_text_multiline("Description", _new_desc);
			}
			g.window_end();
		}
	}

	void level_editor_level_props::draw(sf::RenderTarget &t, time_duration, sf::RenderStates s) const
	{
		t.draw(_background, s);
	}
}
