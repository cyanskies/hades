#ifndef ORTHO_UTILITY_HPP
#define ORTHO_UTILITY_HPP

#include <array>
#include <exception>
#include <vector>

#include "Hades/UniqueId.hpp"

#include "Tiles/tiles.hpp"

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

	namespace transition2
	{
		//NOTE: see http://www.cr31.co.uk/stagecast/wang/2corn.html
		//values for 2-corner transitions
		//named based on which tile corners are 'empty'
		enum TransitionTypes {
			TRANSITION_BEGIN = -1, NONE,
			TOP_RIGHT, BOTTOM_RIGHT, TOP_RIGHT_BOTTOM_RIGHT,
			BOTTOM_LEFT, TOP_RIGHT_BOTTOM_LEFT, BOTTOM_LEFT_RIGHT, TOP_RIGHT_BOTTOM_LEFT_RIGHT,
			TOP_LEFT, TOP_LEFT_RIGHT, TOP_LEFT_BOTTOM_RIGHT, TOP_LEFT_RIGHT_BOTTOM_RIGHT, TOP_LEFT_BOTTOM_LEFT,
			TOP_LEFT_RIGHT_BOTTOM_LEFT, TOP_LEFT_BOTTOM_LEFT_RIGHT,
			ALL, TRANSITION_END
		};
	}

	enum class Corner { TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT };
	bool IsTerrainInCorner(Corner c, transition2::TransitionTypes t);
	using tile_corners = std::array<const resources::terrain*, 4>;
	tile_corners GetCornerData(sf::Vector2u pos, const std::vector<const resources::terrain*> &vert_data, tiles::tile_count_t width);

	//corners: flase if contains terrain, true if empty
	transition2::TransitionTypes PickTransition(const std::array<bool, 4> &corners);
	transition2::TransitionTypes GetTransitionType(tiles::tile, const resources::terrain*);

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
