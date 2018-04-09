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

	hades::data::UniqueId TerrainInCorner(const tile& t, Corner corner);

	namespace transition2
	{
		namespace transition_corners
		{
			//list of tile types with terrain2 in their bottom left corner
			static const std::array<TransitionTypes, 8> bottom_left = { BOTTOM_LEFT, TOP_RIGHT_BOTTOM_LEFT, BOTTOM_LEFT_RIGHT,
				TOP_RIGHT_BOTTOM_LEFT_RIGHT, TOP_LEFT_BOTTOM_LEFT, TOP_LEFT_RIGHT_BOTTOM_LEFT, TOP_LEFT_BOTTOM_LEFT_RIGHT, ALL };
			//list of tile types with terrain2 in their top left corner
			static const std::array<TransitionTypes, 8> top_left = { TOP_LEFT, TOP_LEFT_BOTTOM_LEFT, TOP_LEFT_BOTTOM_LEFT_RIGHT, TOP_LEFT_BOTTOM_RIGHT,
				TOP_LEFT_RIGHT, TOP_LEFT_RIGHT_BOTTOM_LEFT, TOP_LEFT_RIGHT_BOTTOM_RIGHT, ALL };
			//list of tile types with terrain2 in their top right corner
			static const std::array<TransitionTypes, 8> top_right = { TOP_RIGHT, TOP_LEFT_RIGHT, TOP_LEFT_RIGHT_BOTTOM_LEFT,
				TOP_LEFT_RIGHT_BOTTOM_RIGHT, TOP_RIGHT_BOTTOM_LEFT, TOP_RIGHT_BOTTOM_LEFT_RIGHT, TOP_RIGHT_BOTTOM_RIGHT, ALL };
			//list of tile types with terrain2 in their bottom right corner
			static const std::array<TransitionTypes, 8> bottom_right = { BOTTOM_RIGHT, BOTTOM_LEFT_RIGHT, TOP_LEFT_BOTTOM_LEFT_RIGHT, TOP_LEFT_BOTTOM_RIGHT,
				TOP_LEFT_RIGHT_BOTTOM_RIGHT, TOP_RIGHT_BOTTOM_RIGHT, TOP_RIGHT_BOTTOM_LEFT_RIGHT, ALL };
		}

		/*
		tile PickTile2Corner(const std::array<hades::data::UniqueId, 4> &corners)
		{
			std::set<hades::data::UniqueId> unique_terrain(corners.begin(), corners.end());

			//remove null or error tiles, to avoid propagating them
			unique_terrain.erase(hades::data::UniqueId::Zero);
			unique_terrain.erase(GetErrorTerrain());

			if (unique_terrain.size() == 1)
			{
				//return a main tile of the terrain found
				//equivelent to NONE or ALL
				auto terrainid = *unique_terrain.begin();
				assert(hades::data::Exists(terrainid));
				auto terrain = hades::data::Get<resources::terrain>(terrainid);
				return RandomTile(terrain->tile);
			}
			//no support for wang 3corner or anything like that
			//also return error if we don't have any terrain_ids for any reason
			else if (unique_terrain.size() != 2)
				return tiles::GetErrorTile();

			auto terrain1 = *unique_terrain.begin(),
				terrain2 = *++unique_terrain.begin();

			//find a transition that has both these terrains
			auto terrain1_obj = hades::data::Get<resources::terrain>(terrain1);

			const resources::terrain_transition *transition = nullptr;

			//no transition exists for this combination
			if (!transition)
				return tiles::GetErrorTile();

			//now work out which terrain type is the background(terrain1)
			if (transition->terrain1 != terrain1)
				std::swap(terrain1, terrain2);

			//count how many terrain2 tiles are nearby
			auto bitIndex = 0u;

			using namespace transition2::transition_corners;

			if (corners[Corner::TOPRIGHT] == terrain2)
				bitIndex += 1;

			if (corners[Corner::BOTTOMRIGHT] == terrain2)
				bitIndex += 2;

			if (corners[Corner::BOTTOMLEFT] == terrain2)
				bitIndex += 4;

			if (corners[Corner::TOPLEFT] == terrain2)
				bitIndex += 8;

			auto &tiles = GetTransition(static_cast<transition2::TransitionTypes>(bitIndex), *transition);

			return RandomTile(tiles);
		}
		*/
		/*
		hades::data::UniqueId TerrainInCorner2(const tile& tile, Corner corner)
		{
			using namespace transition2::transition_corners;

			auto t = GetTerrainInfo(tile);

			switch (corner)
			{
			case TOPRIGHT:
				if (std::find(top_right.begin(), top_right.end(), t.type) != std::end(top_right))
					return t.terrain2;
				else
					return t.terrain;
			case BOTTOMRIGHT:
				if (std::find(bottom_right.begin(), bottom_right.end(), t.type) != std::end(bottom_right))
					return t.terrain2;
				else
					return t.terrain;
			case BOTTOMLEFT:
				if (std::find(bottom_left.begin(), bottom_left.end(), t.type) != std::end(bottom_left))
					return t.terrain2;
				else
					return t.terrain;
			case TOPLEFT:
				if (std::find(top_left.begin(), top_left.end(), t.type) != std::end(top_left))
					return t.terrain2;
				else
					return t.terrain;
			default:
				return hades::data::UniqueId::Zero;
			}
		}
		*/
	}
	/*
	tile PickTile(const std::array<const resources::terrain*, 4> &vertex)
	{
		//check the adjacent tiles to see which terrains they have
		std::array<hades::data::UniqueId, 4> corners;

		auto zero = hades::data::UniqueId::Zero;

		corners[0] = vertex[0] ? vertex[0]->id : zero;
		corners[1] = vertex[1] ? vertex[1]->id : zero;
		corners[2] = vertex[2] ? vertex[2]->id : zero;
		corners[3] = vertex[3] ? vertex[3]->id : zero;

		return transition3::PickTile3Corner(corners);
	}
	*/
	/*
	tile PickTile(const std::array<tiles::tile, 4> &corner_tiles)
	{
		//check the adjacent tiles to see which terrains they have
		std::array<hades::data::UniqueId, 4> corners;

		corners[0] = TerrainInCorner(corner_tiles[0], BOTTOMLEFT);
		corners[1] = TerrainInCorner(corner_tiles[1], TOPLEFT);
		corners[2] = TerrainInCorner(corner_tiles[2], TOPRIGHT);
		corners[3] = TerrainInCorner(corner_tiles[3], BOTTOMRIGHT);

		return transition3::PickTile3Corner(corners);
	}
	*/
	/*
	hades::data::UniqueId TerrainInCorner(const tile& tile, Corner corner)
	{
		auto t = GetTerrainInfo(tile);

		if (t.terrain3 != hades::data::UniqueId::Zero)
			return transition3::TerrainInCorner3(tile, corner);
		else if (t.terrain2 != hades::data::UniqueId::Zero)
			return transition2::TerrainInCorner2(tile, corner);
		else
			return t.terrain;
	}
	*/

	/*
	template<class U = tiles::TileArray , class V = resources::terrain_transition, class W = resources::terrain >
	U& GetTransition(transition2::TransitionTypes type, V& transitions, std::function<W*(hades::UniqueId)> GetResourceFunc, U& default_val)
	{
		using namespace transition2;

		switch (type)
		{
		case NONE:
		{
			auto terrain1 = GetResourceFunc(transitions.terrain1);
			return terrain1->tiles;
		}
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
		case ALL:
		{
			auto terrain2 = GetResourceFunc(transitions.terrain2);
			return terrain2->tiles;
		}
		default:
			LOGWARNING("'Type' passed to GetTransition was outside expected range[0,15] was: " + std::to_string(type));
			
			return default_val;
		}
	}
	*/
	/*
	tiles::TileArray& GetTransition(transition2::TransitionTypes type, resources::terrain_transition& transition, hades::data::data_system *data)
	{
		return GetTransition<tiles::TileArray, resources::terrain_transition, resources::terrain>(type, transition, 
			[data](hades::UniqueId id) {return data->get<resources::terrain>(id); }, resources::GetMutableErrorTileset(data));
	}

	const tiles::TileArray& GetTransition(transition2::TransitionTypes type, const resources::terrain_transition& transition)
	{
		return GetTransition<const tiles::TileArray, const resources::terrain_transition, const resources::terrain>(type,
			transition, hades::data::Get<resources::terrain>, tiles::GetErrorTileset());
	}
	*/
}
