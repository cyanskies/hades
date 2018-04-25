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

	namespace transition2
	{
		//list of tile types with 'empty' in their bottom left corner
		constexpr std::array<TransitionTypes, 8> bottom_left = { BOTTOM_LEFT, TOP_RIGHT_BOTTOM_LEFT, BOTTOM_LEFT_RIGHT,
			TOP_RIGHT_BOTTOM_LEFT_RIGHT, TOP_LEFT_BOTTOM_LEFT, TOP_LEFT_RIGHT_BOTTOM_LEFT, TOP_LEFT_BOTTOM_LEFT_RIGHT, ALL };
		//list of tile types with 'empty in their top left corner
		constexpr std::array<TransitionTypes, 8> top_left = { TOP_LEFT, TOP_LEFT_BOTTOM_LEFT, TOP_LEFT_BOTTOM_LEFT_RIGHT, TOP_LEFT_BOTTOM_RIGHT,
			TOP_LEFT_RIGHT, TOP_LEFT_RIGHT_BOTTOM_LEFT, TOP_LEFT_RIGHT_BOTTOM_RIGHT, ALL };
		//list of tile types with 'empty' in their top right corner
		constexpr std::array<TransitionTypes, 8> top_right = { TOP_RIGHT, TOP_LEFT_RIGHT, TOP_LEFT_RIGHT_BOTTOM_LEFT,
			TOP_LEFT_RIGHT_BOTTOM_RIGHT, TOP_RIGHT_BOTTOM_LEFT, TOP_RIGHT_BOTTOM_LEFT_RIGHT, TOP_RIGHT_BOTTOM_RIGHT, ALL };
		//list of tile types with 'empty' in their bottom right corner
		constexpr std::array<TransitionTypes, 8> bottom_right = { BOTTOM_RIGHT, BOTTOM_LEFT_RIGHT, TOP_LEFT_BOTTOM_LEFT_RIGHT, TOP_LEFT_BOTTOM_RIGHT,
			TOP_LEFT_RIGHT_BOTTOM_RIGHT, TOP_RIGHT_BOTTOM_RIGHT, TOP_RIGHT_BOTTOM_LEFT_RIGHT, ALL };
	}

	bool IsTerrainInCorner(Corner c, transition2::TransitionTypes t)
	{
		using namespace transition2;
		//test to see if the corner contains 'empty'
		//if not, then return true
		switch (c)
		{
			case Corner::TOP_LEFT:
				return std::find(std::begin(top_left), std::end(top_left), t) == std::end(top_left);
			case Corner::TOP_RIGHT:
				return std::find(std::begin(top_right), std::end(top_right), t) == std::end(top_right);
			case Corner::BOTTOM_LEFT:
				return std::find(std::begin(bottom_left), std::end(bottom_left), t) == std::end(bottom_left);
			case Corner::BOTTOM_RIGHT:
				return std::find(std::begin(bottom_right), std::end(bottom_right), t) == std::end(bottom_right);
			default:
				throw exception("Unexpected value for enum \"Corner\"");
		}
	}

	tile_corners GetCornerData(sf::Vector2u pos, const TerrainVertex &vert_data, tiles::tile_count_t width)
	{
		//top left
		const auto tl_pos = tiles::FlatPosition(pos, width);
		//top right
		const auto tr_pos = tl_pos + 1;
		//bottom left
		++pos.y;
		const auto bl_pos = tiles::FlatPosition(pos, width);
		//bottom right
		const auto br_pos = bl_pos + 1;

		return { vert_data[tl_pos], vert_data[tr_pos], vert_data[bl_pos], vert_data[br_pos] };
	}

	bool InVec(const std::vector<tile> &v, tile t)
	{
		return std::find(std::begin(v), std::end(v), t) != std::end(v);
	}

	transition2::TransitionTypes PickTransition(const std::array<bool, 4> &corners)
	{
		using transition2::TransitionTypes;
		hades::types::uint8 type = 0u;

		if (!corners[0])
			type += 8u;
		if (!corners[1])
			type += 1u;
		if (!corners[2])
			type += 4u;
		if (!corners[3])
			type += 2u;

		return static_cast<TransitionTypes>(type);
	}

	transition2::TransitionTypes GetTransitionType(tile t, const resources::terrain *terr)
	{
		//create array of tile lists for this terrain,
		//the arrays are presented in the numerical order of the TransitionTypes enum
		const std::array<const std::vector<tile>*, 15> tile_arrays = { &terr->full, &terr->bottom_left_corner, &terr->top_left_corner, &terr->left,
			&terr->top_right_corner, &terr->left_diagonal, &terr->top, &terr->top_left_circle, &terr->bottom_right_corner,
			&terr->bottom, &terr->right_diagonal, &terr->bottom_left_circle, &terr->right, &terr->bottom_right_circle, &terr->top_right_circle
		};

		//find the array that contains the tile
		auto pos = std::find_if(std::begin(tile_arrays), std::end(tile_arrays), [t](const std::vector<tile> *t_arr) {
			assert(t_arr);
			return InVec(*t_arr, t);
		});

		//the distance between begin and the correct array should be equal to the correct transitiontype
		return static_cast<transition2::TransitionTypes>(std::distance(std::begin(tile_arrays), pos));
	}

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
		case ALL:
			return transitions.full;
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

	const tiles::TileArray& GetTransitionConst(transition2::TransitionTypes type, const resources::terrain& terrain)
	{
		return GetTransition<const tiles::TileArray, const resources::terrain>(type, terrain);
	}
}
