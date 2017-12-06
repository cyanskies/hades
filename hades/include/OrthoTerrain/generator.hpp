#ifndef ORTHO_GENERATOR_HPP
#define ORTHO_GENERATOR_HPP

#include "OrthoTerrain/resources.hpp"

#include "Tiles/tiles.hpp"

namespace ortho_terrain
{
	namespace generator
	{
		//returns a blank map covered in the specified terrain
		tiles::MapData Blank(const sf::Vector2u &size, const resources::terrain *terrain);
	}
}

#endif // !ORTHO_GENERATOR_HPP
