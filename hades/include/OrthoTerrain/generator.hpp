#ifndef ORTHO_GENERATOR_HPP
#define ORTHO_GENERATOR_HPP

#include "OrthoTerrain/terrain.hpp"

namespace ortho_terrain
{
	namespace generator
	{
		//returns a blank map covered in the specified terrain
		MapData Blank(const sf::Vector2u &size, const resources::terrain *terrain);
	}
}

#endif // !ORTHO_GENERATOR_HPP
