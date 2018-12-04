#include "level_editor_grid.hpp"

#include "hades/properties.hpp"
#include "..\include\hades\level_editor_grid.hpp"

void hades::create_level_editor_grid_variables()
{
	console::create_property(cvars::editor_grid, cvars::default_value::editor_grid);
}

void hades::level_editor_grid::level_load(const level &l)
{
	_grid.set_all({ {l.map_x, l.map_y}, 8.f });
}
