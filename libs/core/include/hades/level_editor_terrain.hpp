#ifndef HADES_LEVEL_EDITOR_TERRAIN_HPP
#define HADES_LEVEL_EDITOR_TERRAIN_HPP

#include "hades/data.hpp"
#include "hades/level_editor_tiles.hpp"
#include "hades/terrain_map.hpp"

namespace hades
{
	void register_level_editor_terrain_resources(data::data_manager&);

	class level_editor_terrain final : public level_editor_tiles
	{
	};
}

#endif //!HADES_LEVEL_EDITOR_TERRAIN_HPP
