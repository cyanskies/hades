#include "OrthoTerrain/generator.hpp"

namespace ortho_terrain
{
	namespace generator
	{
		//returns a blank map covered in the specified terrain
		MapData Blank(const sf::Vector2u &size, const resources::terrain *terrain)
		{
			assert(terrain);
			TileArray tiles(size.x * size.y);

			for (TileArray::size_type i = 0; i < tiles.size(); ++i)
				tiles[i] = RandomTile(terrain->tiles);

			return { tiles, size.x };
		}
	}
}
