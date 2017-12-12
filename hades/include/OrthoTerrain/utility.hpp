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
		struct terrain_transition3;
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

	class error_tileset_unavailable : public std::exception
	{
	public:
		using std::exception::exception;
		using std::exception::what;
	};

	//throws error_tileset_unavailable
	// throws if an error tileset hasn't been assigned to the tile-settings or terrain-settings object
	// NOTE: only throws for the non-const version
	tiles::TileArray& GetTransition(transition2::TransitionTypes type, resources::terrain_transition& transition, hades::data::data_manager*);
	const tiles::TileArray& GetTransition(transition2::TransitionTypes type, const resources::terrain_transition& transition);

	namespace transition3
	{
		//NOTE: see http://www.cr31.co.uk/stagecast/wang/3corn.html
		//naming scheme
		// TX_TY_CORNER_TU_CORNER
		//where X is the background terrain
		//where Y and U are Other terrains
		//where CORNER is the corners that are filled with the preceeding terrain(see TransitionTypes above
		enum Types {
			//Terrain1 background, terrain2 corners
			T1_NONE,
			T1_T2_TOP_RIGHT, T1_T2_BOTTOM_RIGHT = 3, T1_T2_TOP_RIGHT_BOTTOM_RIGHT, T1_T2_BOTTOM_LEFT = 9,
			T1_T2_TOP_RIGHT_BOTTOM_LEFT, T1_T2_BOTTOM_LEFT_RIGHT = 12, T1_T2_TOP_RIGHT_BOTTOM_LEFT_RIGHT, T1_T2_TOP_LEFT = 27,
			T1_T2_TOP_LEFT_RIGHT, T1_T2_TOP_LEFT_BOTTOM_RIGHT = 30, T1_T2_TOP_LEFT_RIGHT_BOTTOM_RIGHT, T1_T2_TOP_LEFT_BOTTOM_LEFT = 36,
			T1_T2_TOP_LEFT_RIGHT_BOTTOM_LEFT, T1_T2_TOP_LEFT_BOTTOM_LEFT_RIGHT = 39, T2_NONE,
			//terrain2 background, terrain3 corners
			T2_T3_TOP_RIGHT, T2_T3_BOTTOM_RIGHT = 43, T2_T3_TOP_RIGHT_BOTTOM_RIGHT, T2_T3_BOTTOM_LEFT = 49,
			T2_T3_TOP_RIGHT_BOTTOM_LEFT, T2_T3_BOTTOM_LEFT_RIGHT = 52, T2_T3_TOP_RIGHT_BOTTOM_LEFT_RIGHT, T2_T3_TOP_LEFT = 67,
			T2_T3_TOP_LEFT_RIGHT, T2_T3_TOP_LEFT_BOTTOM_RIGHT = 70, T2_T3_TOP_LEFT_RIGHT_BOTTOM_RIGHT, T2_T3_TOP_LEFT_BOTTOM_LEFT = 76,
			T2_T3_TOP_LEFT_RIGHT_BOTTOM_LEFT, T2_T3_TOP_LEFT_BOTTOM_LEFT_RIGHT = 79, T3_NONE,
			//terrain1 background, terrain3 corners
			T1_T3_TOP_RIGHT = 2, T1_T3_BOTTOM_RIGHT = 6, T1_T3_TOP_RIGHT_BOTTOM_RIGHT = 8, T1_T3_BOTTOM_LEFT = 18,
			T1_T3_TOP_RIGHT_BOTTOM_LEFT = 20, T1_T3_BOTTOM_LEFT_RIGHT = 24, T1_T3_TOP_RIGHT_BOTTOM_LEFT_RIGHT = 26, T1_T3_TOP_LEFT = 54,
			T1_T3_TOP_LEFT_RIGHT = 56, T1_T3_TOP_LEFT_BOTTOM_RIGHT = 60, T1_T3_TOP_LEFT_RIGHT_BOTTOM_RIGHT = 62, T1_T3_TOP_LEFT_BOTTOM_LEFT = 72,
			T1_T3_TOP_LEFT_RIGHT_BOTTOM_LEFT = 74, T1_T3_TOP_LEFT_BOTTOM_LEFT_RIGHT = 78,
			//terrain1 or 2 background, terrain 2 or 3 corners
			//5 through 25
			T1_T2_BOTTOM_RIGHT_T3_TOP_RIGHT = 5, T1_T2_TOP_RIGHT_T3_BOTTOM_RIGHT = 7, T1_T2_BOTTOM_LEFT_T3_TOP_RIGHT = 11, T1_T2_BOTTOM_LEFT_RIGHT_T3_TOP_RIGHT = 14,
			T1_T2_BOTTOM_LEFT_T3_BOTTOM_RIGHT, T1_T2_TOP_RIGHT_BOTTOM_LEFT_T3_BOTTOM_RIGHT, T1_T2_BOTTOM_LEFT_T3_TOP_RIGHT_BOTTOM_RIGHT, T1_T2_TOP_RIGHT_T3_BOTTOM_LEFT = 19,
			T1_T2_BOTTOM_RIGHT_T3_BOTTOM_LEFT = 21, T1_T2_TOP_RIGHT_BOTTOM_RIGHT_T3_BOTTOM_LEFT, T1_T2_BOTTOM_RIGHT_T3_TOP_RIGHT_BOTTOM_LEFT, T1_T2_TOP_RIGHT_T3_BOTTOM_LEFT_RIGHT = 25,
			//29 through 51
			T1_T2_TOP_LEFT_T3_TOP_RIGHT = 29, T1_T2_TOP_LEFT_BOTTOM_RIGHT_T3_TOP_RIGHT = 32, T1_T2_TOP_LEFT_T3_BOTTOM_RIGHT, T1_T2_TOP_LEFT_RIGHT_T3_BOTTOM_RIGHT,
			T1_T2_TOP_LEFT_T3_TOP_RIGHT_BOTTOM_RIGHT, T1_T2_TOP_LEFT_BOTTOM_LEFT_T3_TOP_RIGHT = 38, T1_T2_TOP_LEFT_BOTTOM_LEFT_T3_BOTTOM_RIGHT = 42, T1_T2_TOP_LEFT_T3_BOTTOM_LEFT = 45,
			T1_T2_TOP_LEFT_RIGHT_T3_BOTTOM_LEFT, T1_T2_TOP_LEFT_T3_TOP_RIGHT_BOTTOM_LEFT, T1_T2_TOP_LEFT_BOTTOM_RIGHT_T3_BOTTOM_LEFT, T1_T2_TOP_LEFT_T3_BOTTOM_LEFT_RIGHT = 51,
			//55 through 75
			T1_T2_TOP_RIGHT_T3_TOP_LEFT = 55, T1_T2_BOTTOM_RIGHT_T3_TOP_LEFT = 57, T1_T2_TOP_RIGHT_BOTTOM_RIGHT_T3_TOP_LEFT, T1_T2_BOTTOM_RIGHT_T3_TOP_LEFT_RIGHT,
			T1_T2_TOP_RIGHT_T3_TOP_LEFT_BOTTOM_RIGHT = 61, T1_T2_BOTTOM_LEFT_T3_TOP_LEFT = 63, T1_T2_TOP_RIGHT_BOTTOM_LEFT_T3_TOP_LEFT, T1_T2_BOTTOM_LEFT_T3_TOP_LEFT_RIGHT,
			T1_T2_BOTTOM_LEFT_RIGHT_T3_TOP_LEFT, T1_T2_BOTTOM_LEFT_T3_TOP_LEFT_BOTTOM_RIGHT = 69, T1_T2_TOP_RIGHT_T3_TOP_LEFT_BOTTOM_LEFT = 73, T1_T2_BOTTOM_RIGHT_T3_TOP_LEFT_BOTTOM_LEFT = 75
		};
	}

	//throws error_tileset_unavailable as above
	tiles::TileArray& GetTransition(transition3::Types type, resources::terrain_transition3& transition, hades::data::data_manager*);
	const tiles::TileArray& GetTransition(transition3::Types type, const resources::terrain_transition3& transition);
}

#endif // !ORTHO_UTILITY_HPP
