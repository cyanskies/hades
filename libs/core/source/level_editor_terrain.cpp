#include "hades/level_editor_terrain.hpp"

namespace hades
{
	void register_level_editor_terrain_resources(data::data_manager &d)
	{
		register_terrain_map_resources(d);
		register_level_editor_tiles_resources(d);
	}
}