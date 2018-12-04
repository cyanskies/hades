#include "level_editor_grid.hpp"

#include "hades/properties.hpp"

void hades::create_level_editor_grid_variables()
{
	console::create_property(cvars::editor_grid, cvars::default_value::editor_grid);
}
