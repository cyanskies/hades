#ifndef ORTHO_UTILITY_HPP
#define ORTHO_UTILITY_HPP

#include <array>
#include <exception>
#include <vector>

#include "Hades/simple_resources.hpp"
#include "Hades/Types.hpp"

#include "Tiles/resources.hpp"
#include "Tiles/tiles.hpp"

//contains utility functions for arranging and picking tiles from transitions
namespace ortho_terrain
{
	//forward declarations
	namespace resources
	{
		struct terrain;
		struct terrain_transition;
	}

	//returns a random tile from the selection
	tiles::tile RandomTile(const std::vector<tiles::tile>& tiles);

	//Picks a tile that seemlessly fits between the 4 corner verticies
	tiles::tile PickTile(const std::array<const resources::terrain*, 4> &vertex);
	//Picks a tile that seemlessly fits between the 4 corner tiles provided
	tiles::tile PickTile(const std::array<tiles::tile, 4> &corner_tiles);

	//returns the terrain type in a corner of the tile
	//NOTE: the numerical value of these is important
	// they must remain in the range 0, 3
	// as they are used by the AsVertex/AsTiles functions in terrain.cpp
	// and the pick tile functions above
	enum Corner { TOPLEFT, TOPRIGHT, BOTTOMLEFT, BOTTOMRIGHT, CORNER_LAST};
	hades::data::UniqueId TerrainInCorner(const tiles::tile& t, Corner corner);

	namespace transition2
	{
		//NOTE: see http://www.cr31.co.uk/stagecast/wang/2corn.html
		//values for 2-corner transitions
		enum TransitionTypes {
			NONE,
			TOP_RIGHT, BOTTOM_RIGHT, TOP_RIGHT_BOTTOM_RIGHT,
			BOTTOM_LEFT, TOP_RIGHT_BOTTOM_LEFT, BOTTOM_LEFT_RIGHT, TOP_RIGHT_BOTTOM_LEFT_RIGHT,
			TOP_LEFT, TOP_LEFT_RIGHT, TOP_LEFT_BOTTOM_RIGHT, TOP_LEFT_RIGHT_BOTTOM_RIGHT, TOP_LEFT_BOTTOM_LEFT,
			TOP_LEFT_RIGHT_BOTTOM_LEFT, TOP_LEFT_BOTTOM_LEFT_RIGHT,
			ALL
		};
	}

	tiles::TileArray& GetTransition(transition2::TransitionTypes type, resources::terrain_transition& transition, hades::data::data_manager*);
	const tiles::TileArray& GetTransition(transition2::TransitionTypes type, const resources::terrain_transition& transition);
}

#endif // !ORTHO_UTILITY_HPP
