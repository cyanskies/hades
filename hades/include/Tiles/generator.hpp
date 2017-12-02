#ifndef TILES_GENERATOR_HPP
#define TILES_GENERATOR_HPP

#include "Tiles/tiles.hpp"

namespace tiles
{
	namespace generator
	{
		//returns a blank map covered in the specified terrain
		MapData Blank(const sf::Vector2u &size, tile tile);
	}
}

#endif // !TILES_GENERATOR_HPP
