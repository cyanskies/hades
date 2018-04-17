#ifndef ORTHO_UTILITY_HPP
#define ORTHO_UTILITY_HPP

#include <array>
#include <exception>
#include <vector>

#include "Hades/UniqueId.hpp"

namespace tiles
{
	struct tile;
}

//contains utility functions for arranging and picking tiles from transitions
namespace ortho_terrain
{
	//forward declarations
	namespace resources
	{
		struct terrain;
	}

	//returns a random tile from the selection
	tiles::tile RandomTile(const std::vector<tiles::tile>& tiles);

	//Picks a tile that seemlessly fits between the 4 corner verticies
	tiles::tile PickTile(const std::array<const resources::terrain*, 4> &vertex);

	namespace transition2
	{
		//NOTE: see http://www.cr31.co.uk/stagecast/wang/2corn.html
		//values for 2-corner transitions
		enum TransitionTypes {
			TRANSITION_BEGIN = -1, NONE,
			TOP_RIGHT, BOTTOM_RIGHT, TOP_RIGHT_BOTTOM_RIGHT,
			BOTTOM_LEFT, TOP_RIGHT_BOTTOM_LEFT, BOTTOM_LEFT_RIGHT, TOP_RIGHT_BOTTOM_LEFT_RIGHT,
			TOP_LEFT, TOP_LEFT_RIGHT, TOP_LEFT_BOTTOM_RIGHT, TOP_LEFT_RIGHT_BOTTOM_RIGHT, TOP_LEFT_BOTTOM_LEFT,
			TOP_LEFT_RIGHT_BOTTOM_LEFT, TOP_LEFT_BOTTOM_LEFT_RIGHT,
			ALL, TRANSITION_END
		};
	}

	class out_of_range : public std::out_of_range
	{
	public:
		using std::out_of_range::out_of_range;
	};

	//both of these can throw out_of_range
	std::vector<tiles::tile>& GetTransition(transition2::TransitionTypes type, resources::terrain& terrain);
	const std::vector<tiles::tile>& GetTransitionConst(transition2::TransitionTypes type, const resources::terrain& terrain);
}

#endif // !ORTHO_UTILITY_HPP
