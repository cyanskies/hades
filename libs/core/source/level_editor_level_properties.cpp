#include "hades/level_editor_level_properties.hpp"

#include "hades/gui.hpp"

namespace hades
{
	void make_level_detail_window(gui &g, std::string name, std::string description)
	{
		if (g.window_begin("level details"))
		{

		}
		g.window_end();
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
}
