#include "hades/level_editor_level_properties.hpp"

#include "hades/gui.hpp"

namespace hades
{
	void level_editor_level_props::gui_update(gui &g)
	{
		g.main_menubar_begin();
		if (g.menu_begin("level"))
		{
			g.menu_end();
		}

		g.main_menubar_end();
	}

	void level_editor_level_props::gui_menubar(menu_functions menu)
	{
	}
}
