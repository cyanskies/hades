#include "hades/level_editor_objects.hpp"

namespace hades
{
	void level_editor_objects::gui_toolbar(toolbar_functions toolbar)
	{
		if (toolbar.button("selector"))
		{
			_brush_type = brush_type::object_selector;
			activate_brush();
		}
	}
}