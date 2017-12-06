#include "OrthoTerrain/generator.hpp"

namespace ortho_terrain
{
	namespace generator
	{
		//returns a blank map covered in the specified terrain
		tiles::MapData Blank(const sf::Vector2u &size, const resources::terrain *terrain)
		{
			assert(terrain);
			tiles::TileArray tiles(size.x * size.y);

			for (tiles::TileArray::size_type i = 0; i < tiles.size(); ++i)
				tiles[i] = RandomTile(terrain->tiles);

			return { tiles, size.x };
		}
	}
}
