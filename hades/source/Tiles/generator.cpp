#include "Tiles/generator.hpp"

namespace tiles
{
	namespace generator
	{
		//returns a blank map covered in the specified terrain
		MapData Blank(const sf::Vector2u &size, tile tile)
		{
			TileArray tiles(size.x * size.y, tile);
			return { tiles, size.x };
		}
	}
}
