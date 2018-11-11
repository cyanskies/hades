#include "hades/level_editor_level_properties.hpp"

#include "hades/gui.hpp"

namespace hades
{
	static void make_level_detail_window(gui &g, std::string name, std::string description)
	{
		if (g.window_begin("level details"))
		{

		}
		g.window_end();
	}

	void level_editor_level_props::level_load(const level &l)
	{
		_name_input = l.name;
		_desc_input = l.description;

		_background.setSize({ static_cast<float>(l.map_x),
							  static_cast<float>(l.map_y) });

		_background.setFillColor(sf::Color::Black);
	}

	void level_editor_level_props::gui_update(gui &g)
	{
		using namespace std::string_view_literals;
		g.main_menubar_begin();
		if (g.menu_begin("level"sv))
		{
			if (g.menu_item("details..."sv))
				make_level_detail_window(g, _level_name, _level_desc);

			g.menu_end();
		}

		g.main_menubar_end();
	}

	void level_editor_level_props::draw(sf::RenderTarget &t, time_duration, sf::RenderStates s) const
	{
		t.draw(_background, s);
	}
}
