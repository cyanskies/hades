#include "hades/level_editor_objects.hpp"

#include "hades/gui.hpp"

namespace hades
{
	void level_editor_objects::gui_update(gui &g)
	{
		//toolbar buttons
		g.main_toolbar_begin();
		g.toolbar_button("separator");
		g.main_toolbar_end();
	}
}