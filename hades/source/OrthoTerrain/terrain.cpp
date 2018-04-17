#include "OrthoTerrain/terrain.hpp"

#include <array>

#include "SFML/Graphics/RenderTarget.hpp"

#include "Hades/Data.hpp"
#include "Hades/Utility.hpp"

#include "OrthoTerrain/utility.hpp"
#include "OrthoTerrain/resources.hpp"

#include "Tiles/tiles.hpp"

namespace ortho_terrain
{
	void EnableTerrain(hades::data::data_system *d)
	{
		tiles::EnableTiles(d);
		RegisterOrthoTerrainResources(d);
	}

	namespace
	{
		static const tiles::VertexArray::size_type VertexPerTile = 6;
	}

	using tiles::MapData;
	using tiles::tile_count_t;
	using tiles::TileArray;
	using tiles::tile;
}
