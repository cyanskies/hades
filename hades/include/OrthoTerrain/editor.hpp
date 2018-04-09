#ifndef ORTHO_EDITOR_HPP
#define ORTHO_EDITOR_HPP

#include "Hades/State.hpp"
#include "Hades/Types.hpp"

#include "Tiles/editor.hpp"

namespace ortho_terrain
{
	namespace editor
	{
		enum EditMode : objects::editor::EditMode_t
		{
			TERRAIN = tiles::editor::EditMode::TILE_MODE_END, TERRAIN_MODE_END
		};

		enum class TerrainEditMode {
			NONE, //no tile selected
			TERRAIN, //draw a specific tile, no cleanup
		};
	}

	class terrain_editor : public tiles::tile_editor
	{};
}

#endif // !ORTHO_EDITOR_HPP
