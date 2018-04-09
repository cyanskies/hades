#include "OrthoTerrain/utility.hpp"

#include "Hades/Data.hpp"
#include "Hades/data_system.hpp"
#include "Hades/Utility.hpp"

#include "OrthoTerrain/resources.hpp"

#include "OrthoTerrain/terrain.hpp"
#include "Tiles/tiles.hpp"

namespace ortho_terrain
{
	using tiles::tile;

	tile RandomTile(const std::vector<tile>& tiles)
	{
		if (tiles.empty())
			return tiles::GetErrorTile();

		auto max = tiles.size() - 1;
		auto pos = hades::random(0u, max);
		return tiles[pos];
	}

	template<class U = tiles::TileArray, class W = resources::terrain >
	U& GetTransition(transition2::TransitionTypes type, W& transitions)
	{
		using namespace transition2;

		switch (type)
		{
		case NONE:
			return transitions.tile;
		case TOP_RIGHT:
			return transitions.top_right_corner;
		case BOTTOM_RIGHT:
			return transitions.bottom_right_corner;
		case TOP_RIGHT_BOTTOM_RIGHT:
			return transitions.left;
		case BOTTOM_LEFT:
			return transitions.bottom_left_corner;
		case TOP_RIGHT_BOTTOM_LEFT:
			return transitions.left_diagonal;
		case BOTTOM_LEFT_RIGHT:
			return transitions.top;
		case TOP_RIGHT_BOTTOM_LEFT_RIGHT:
			return transitions.bottom_right_circle;
		case TOP_LEFT:
			return transitions.top_left_corner;
		case TOP_LEFT_RIGHT:
			return transitions.bottom;
		case TOP_LEFT_BOTTOM_RIGHT:
			return transitions.right_diagonal;
		case TOP_LEFT_RIGHT_BOTTOM_RIGHT:
			return transitions.bottom_left_circle;
		case TOP_LEFT_BOTTOM_LEFT:
			return transitions.right;
		case TOP_LEFT_RIGHT_BOTTOM_LEFT:
			return transitions.top_left_circle;
		case TOP_LEFT_BOTTOM_LEFT_RIGHT:
			return transitions.top_right_circle;
		default:
			const auto message = "'Type' passed to GetTransition was outside expected range[0,14] was: " + std::to_string(type);
			throw out_of_range(message);
		}
	}

	tiles::TileArray& GetTransition(transition2::TransitionTypes type, resources::terrain& terrain)
	{
		return GetTransition<tiles::TileArray, resources::terrain>(type, terrain);
	}

	const tiles::TileArray& GetTransition(transition2::TransitionTypes type, const resources::terrain& terrain)
	{
		return GetTransition<const tiles::TileArray, const resources::terrain>(type, terrain);
	}
}
