#include "OrthoTerrain/utility.hpp"

#include "Hades/data_manager.hpp"
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
				assert(hades::data_manager->exists(terrainid));
				auto terrain = hades::data_manager->get<resources::terrain>(terrainid);
				return RandomTile(terrain->tiles);
			}
			//no support for wang 3corner or anything like that
			//also return error if we don't have any terrain_ids for any reason
			else if (unique_terrain.size() != 2)
				return tiles::GetErrorTile();

			auto terrain1 = *unique_terrain.begin(),
				terrain2 = *++unique_terrain.begin();

			//find a transition that has both these terrains
			auto terrain1_obj = hades::data_manager->get<resources::terrain>(terrain1);

			resources::terrain_transition *transition = nullptr;

			for (auto &transid : terrain1_obj->transitions)
			{
				auto trans = hades::data_manager->get<resources::terrain_transition>(transid);
				if ((trans->terrain1 == terrain1 && trans->terrain2 == terrain2) ||
					(trans->terrain1 == terrain2 && trans->terrain2 == terrain1))
				{
					transition = trans;
					break;
				}
			}

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
	}

	namespace transition3
	{
		namespace transition_corners
		{
			static const std::array<Types, 27> terrain2_bottom_left = {
				//2 corner transitions
				T1_T2_BOTTOM_LEFT, T1_T2_TOP_RIGHT_BOTTOM_LEFT, T1_T2_BOTTOM_LEFT_RIGHT, T1_T2_TOP_RIGHT_BOTTOM_LEFT_RIGHT,
				T1_T2_TOP_LEFT_BOTTOM_LEFT, T1_T2_TOP_LEFT_RIGHT_BOTTOM_LEFT, T1_T2_TOP_LEFT_BOTTOM_LEFT_RIGHT, T2_NONE,
				T2_T3_TOP_RIGHT, T2_T3_BOTTOM_RIGHT, T2_T3_TOP_RIGHT_BOTTOM_RIGHT,
				T2_T3_TOP_LEFT,	T2_T3_TOP_LEFT_RIGHT, T2_T3_TOP_LEFT_BOTTOM_RIGHT, T2_T3_TOP_LEFT_RIGHT_BOTTOM_RIGHT,
				//3 corner transitions
				T1_T2_BOTTOM_LEFT_T3_TOP_RIGHT, T1_T2_BOTTOM_LEFT_RIGHT_T3_TOP_RIGHT, T1_T2_BOTTOM_LEFT_T3_BOTTOM_RIGHT, T1_T2_TOP_RIGHT_BOTTOM_LEFT_T3_BOTTOM_RIGHT,
				T1_T2_BOTTOM_LEFT_T3_TOP_RIGHT_BOTTOM_RIGHT, T1_T2_TOP_LEFT_BOTTOM_LEFT_T3_TOP_RIGHT, T1_T2_TOP_LEFT_BOTTOM_LEFT_T3_BOTTOM_RIGHT, T1_T2_BOTTOM_LEFT_T3_TOP_LEFT,
				T1_T2_TOP_RIGHT_BOTTOM_LEFT_T3_TOP_LEFT, T1_T2_BOTTOM_LEFT_T3_TOP_LEFT_RIGHT, T1_T2_BOTTOM_LEFT_RIGHT_T3_TOP_LEFT, T1_T2_BOTTOM_LEFT_T3_TOP_LEFT_BOTTOM_RIGHT };

			static const std::array<Types, 27> terrain3_bottom_left = {
				//2 corner transistions
				T2_T3_BOTTOM_LEFT, T2_T3_TOP_RIGHT_BOTTOM_LEFT, T2_T3_BOTTOM_LEFT_RIGHT, T2_T3_TOP_RIGHT_BOTTOM_LEFT_RIGHT,
				T2_T3_TOP_LEFT_BOTTOM_LEFT, T2_T3_TOP_LEFT_RIGHT_BOTTOM_LEFT, T2_T3_TOP_LEFT_BOTTOM_LEFT_RIGHT, T3_NONE,
				T1_T3_BOTTOM_LEFT, T1_T3_TOP_RIGHT_BOTTOM_LEFT, T1_T3_BOTTOM_LEFT_RIGHT, T1_T3_TOP_RIGHT_BOTTOM_LEFT_RIGHT,
				T1_T3_TOP_LEFT_BOTTOM_LEFT, T1_T3_TOP_LEFT_RIGHT_BOTTOM_LEFT, T1_T3_TOP_LEFT_BOTTOM_LEFT_RIGHT,
				//3 corner transitions
				T1_T2_TOP_RIGHT_T3_BOTTOM_LEFT, T1_T2_BOTTOM_RIGHT_T3_BOTTOM_LEFT, T1_T2_TOP_RIGHT_BOTTOM_RIGHT_T3_BOTTOM_LEFT, T1_T2_BOTTOM_RIGHT_T3_TOP_RIGHT_BOTTOM_LEFT,
				T1_T2_TOP_RIGHT_T3_BOTTOM_LEFT_RIGHT, T1_T2_TOP_LEFT_T3_BOTTOM_LEFT, T1_T2_TOP_LEFT_RIGHT_T3_BOTTOM_LEFT, T1_T2_TOP_LEFT_T3_TOP_RIGHT_BOTTOM_LEFT,
				T1_T2_TOP_LEFT_BOTTOM_RIGHT_T3_BOTTOM_LEFT, T1_T2_TOP_LEFT_T3_BOTTOM_LEFT_RIGHT, T1_T2_TOP_RIGHT_T3_TOP_LEFT_BOTTOM_LEFT , T1_T2_BOTTOM_RIGHT_T3_TOP_LEFT_BOTTOM_LEFT };

			static const std::array<Types, 27> terrain2_top_left = {
				//2 corner transitions
				T1_T2_TOP_LEFT, T1_T2_TOP_LEFT_RIGHT, T1_T2_TOP_LEFT_BOTTOM_RIGHT, T1_T2_TOP_LEFT_RIGHT_BOTTOM_RIGHT,
				T1_T2_TOP_LEFT_BOTTOM_LEFT, T1_T2_TOP_LEFT_RIGHT_BOTTOM_LEFT, T1_T2_TOP_LEFT_BOTTOM_LEFT_RIGHT, T2_NONE,
				T2_T3_TOP_RIGHT, T2_T3_BOTTOM_RIGHT, T2_T3_TOP_RIGHT_BOTTOM_RIGHT, T2_T3_BOTTOM_LEFT,
				T2_T3_TOP_RIGHT_BOTTOM_LEFT, T2_T3_BOTTOM_LEFT_RIGHT, T2_T3_TOP_RIGHT_BOTTOM_LEFT_RIGHT,
				//3 corner transitions
				T1_T2_TOP_LEFT_T3_TOP_RIGHT, T1_T2_TOP_LEFT_BOTTOM_RIGHT_T3_TOP_RIGHT, T1_T2_TOP_LEFT_T3_BOTTOM_RIGHT, T1_T2_TOP_LEFT_RIGHT_T3_BOTTOM_RIGHT,
				T1_T2_TOP_LEFT_T3_TOP_RIGHT_BOTTOM_RIGHT, T1_T2_TOP_LEFT_BOTTOM_LEFT_T3_TOP_RIGHT, T1_T2_TOP_LEFT_BOTTOM_LEFT_T3_BOTTOM_RIGHT, T1_T2_TOP_LEFT_T3_BOTTOM_LEFT,
				T1_T2_TOP_LEFT_RIGHT_T3_BOTTOM_LEFT, T1_T2_TOP_LEFT_T3_TOP_RIGHT_BOTTOM_LEFT, T1_T2_TOP_LEFT_BOTTOM_RIGHT_T3_BOTTOM_LEFT, T1_T2_TOP_LEFT_T3_BOTTOM_LEFT_RIGHT };

			static const std::array<Types, 27> terrain3_top_left = {
				//2 corner transitions 
				T2_T3_TOP_LEFT, T2_T3_TOP_LEFT_RIGHT, T2_T3_TOP_LEFT_BOTTOM_RIGHT, T2_T3_TOP_LEFT_RIGHT_BOTTOM_RIGHT,
				T2_T3_TOP_LEFT_BOTTOM_LEFT,	T2_T3_TOP_LEFT_RIGHT_BOTTOM_LEFT, T2_T3_TOP_LEFT_BOTTOM_LEFT_RIGHT, T3_NONE,
				T1_T3_TOP_LEFT_RIGHT, T1_T3_TOP_LEFT_BOTTOM_RIGHT, T1_T3_TOP_LEFT_RIGHT_BOTTOM_RIGHT, T1_T3_TOP_LEFT_BOTTOM_LEFT,
				T1_T3_TOP_LEFT_RIGHT_BOTTOM_LEFT, T1_T3_TOP_LEFT_BOTTOM_LEFT_RIGHT,
				//3 corner transitions
				T1_T2_TOP_RIGHT_T3_TOP_LEFT, T1_T2_BOTTOM_RIGHT_T3_TOP_LEFT, T1_T2_TOP_RIGHT_BOTTOM_RIGHT_T3_TOP_LEFT, T1_T2_BOTTOM_RIGHT_T3_TOP_LEFT_RIGHT,
				T1_T2_TOP_RIGHT_T3_TOP_LEFT_BOTTOM_RIGHT, T1_T2_BOTTOM_LEFT_T3_TOP_LEFT, T1_T2_TOP_RIGHT_BOTTOM_LEFT_T3_TOP_LEFT, T1_T2_BOTTOM_LEFT_T3_TOP_LEFT_RIGHT,
				T1_T2_BOTTOM_LEFT_RIGHT_T3_TOP_LEFT, T1_T2_BOTTOM_LEFT_T3_TOP_LEFT_BOTTOM_RIGHT, T1_T2_TOP_RIGHT_T3_TOP_LEFT_BOTTOM_LEFT, T1_T2_BOTTOM_RIGHT_T3_TOP_LEFT_BOTTOM_LEFT };

			static const std::array<Types, 27> terrain2_top_right = {
				//2 corner transitions
				T1_T2_TOP_RIGHT, T1_T2_TOP_RIGHT_BOTTOM_RIGHT, T1_T2_TOP_RIGHT_BOTTOM_LEFT, T1_T2_TOP_RIGHT_BOTTOM_LEFT_RIGHT,
				T1_T2_TOP_LEFT_RIGHT, T1_T2_TOP_LEFT_RIGHT_BOTTOM_RIGHT, T1_T2_TOP_LEFT_RIGHT_BOTTOM_LEFT, T2_NONE,
				T2_T3_BOTTOM_RIGHT, T2_T3_BOTTOM_LEFT, T2_T3_BOTTOM_LEFT_RIGHT, T2_T3_TOP_LEFT,
				T2_T3_TOP_LEFT_BOTTOM_RIGHT, T2_T3_TOP_LEFT_BOTTOM_LEFT, T2_T3_TOP_LEFT_BOTTOM_LEFT_RIGHT,
				//3 corner transitions
				T1_T2_TOP_RIGHT_T3_BOTTOM_RIGHT, T1_T2_TOP_RIGHT_BOTTOM_LEFT_T3_BOTTOM_RIGHT, T1_T2_TOP_RIGHT_T3_BOTTOM_LEFT, T1_T2_TOP_RIGHT_BOTTOM_RIGHT_T3_BOTTOM_LEFT,
				T1_T2_TOP_RIGHT_T3_BOTTOM_LEFT_RIGHT, T1_T2_TOP_LEFT_RIGHT_T3_BOTTOM_RIGHT,	T1_T2_TOP_LEFT_RIGHT_T3_BOTTOM_LEFT, T1_T2_TOP_RIGHT_T3_TOP_LEFT,
				T1_T2_TOP_RIGHT_BOTTOM_RIGHT_T3_TOP_LEFT, T1_T2_TOP_RIGHT_T3_TOP_LEFT_BOTTOM_RIGHT, T1_T2_TOP_RIGHT_BOTTOM_LEFT_T3_TOP_LEFT, T1_T2_TOP_RIGHT_T3_TOP_LEFT_BOTTOM_LEFT };

			static const std::array<Types, 27> terrain3_top_right = {
				//2 corner transitions
				T2_T3_TOP_RIGHT, T2_T3_TOP_RIGHT_BOTTOM_RIGHT, T2_T3_TOP_RIGHT_BOTTOM_LEFT, T2_T3_TOP_RIGHT_BOTTOM_LEFT_RIGHT,
				T2_T3_TOP_LEFT_RIGHT, T2_T3_TOP_LEFT_RIGHT_BOTTOM_RIGHT, T2_T3_TOP_LEFT_RIGHT_BOTTOM_LEFT, T3_NONE,
				T1_T3_TOP_RIGHT, T1_T3_TOP_RIGHT_BOTTOM_RIGHT, T1_T3_TOP_RIGHT_BOTTOM_LEFT, T1_T3_TOP_RIGHT_BOTTOM_LEFT_RIGHT,
				T1_T3_TOP_LEFT_RIGHT, T1_T3_TOP_LEFT_RIGHT_BOTTOM_RIGHT, T1_T3_TOP_LEFT_RIGHT_BOTTOM_LEFT,
				//3 corner transitions
				T1_T2_BOTTOM_RIGHT_T3_TOP_RIGHT, T1_T2_BOTTOM_LEFT_T3_TOP_RIGHT, T1_T2_BOTTOM_LEFT_RIGHT_T3_TOP_RIGHT, T1_T2_BOTTOM_LEFT_T3_TOP_RIGHT_BOTTOM_RIGHT,
				T1_T2_BOTTOM_RIGHT_T3_TOP_RIGHT_BOTTOM_LEFT, T1_T2_TOP_LEFT_T3_TOP_RIGHT, T1_T2_TOP_LEFT_BOTTOM_RIGHT_T3_TOP_RIGHT,	T1_T2_TOP_LEFT_T3_TOP_RIGHT_BOTTOM_RIGHT,
				T1_T2_TOP_LEFT_BOTTOM_LEFT_T3_TOP_RIGHT, T1_T2_TOP_LEFT_T3_TOP_RIGHT_BOTTOM_LEFT, T1_T2_BOTTOM_RIGHT_T3_TOP_LEFT_RIGHT,	T1_T2_BOTTOM_LEFT_T3_TOP_LEFT_RIGHT };

			static const std::array<Types, 27> terrain2_bottom_right = {
				//2 corner transitions
				T1_T2_BOTTOM_RIGHT, T1_T2_TOP_RIGHT_BOTTOM_RIGHT, T1_T2_BOTTOM_LEFT_RIGHT, T1_T2_TOP_RIGHT_BOTTOM_LEFT_RIGHT,
				T1_T2_TOP_LEFT_BOTTOM_RIGHT, T1_T2_TOP_LEFT_RIGHT_BOTTOM_RIGHT, T1_T2_TOP_LEFT_BOTTOM_LEFT_RIGHT, T2_NONE,
				T2_T3_TOP_RIGHT, T2_T3_BOTTOM_LEFT, T2_T3_TOP_RIGHT_BOTTOM_LEFT, T2_T3_TOP_LEFT, T2_T3_TOP_LEFT_RIGHT,
				T2_T3_TOP_LEFT_BOTTOM_LEFT,	T2_T3_TOP_LEFT_RIGHT_BOTTOM_LEFT,
				//3 corner transitions
				T1_T2_BOTTOM_RIGHT_T3_TOP_RIGHT, T1_T2_BOTTOM_LEFT_RIGHT_T3_TOP_RIGHT, T1_T2_BOTTOM_RIGHT_T3_BOTTOM_LEFT, T1_T2_TOP_RIGHT_BOTTOM_RIGHT_T3_BOTTOM_LEFT,
				T1_T2_BOTTOM_RIGHT_T3_TOP_RIGHT_BOTTOM_LEFT, T1_T2_TOP_LEFT_BOTTOM_RIGHT_T3_TOP_RIGHT,T1_T2_TOP_LEFT_BOTTOM_RIGHT_T3_BOTTOM_LEFT, T1_T2_BOTTOM_RIGHT_T3_TOP_LEFT,
				T1_T2_TOP_RIGHT_BOTTOM_RIGHT_T3_TOP_LEFT, T1_T2_BOTTOM_RIGHT_T3_TOP_LEFT_RIGHT,	T1_T2_BOTTOM_LEFT_RIGHT_T3_TOP_LEFT, T1_T2_BOTTOM_RIGHT_T3_TOP_LEFT_BOTTOM_LEFT };

			static const std::array<Types, 27> terrain3_bottom_right = {
				//2 corner transitions
				T2_T3_BOTTOM_RIGHT, T2_T3_TOP_RIGHT_BOTTOM_RIGHT, T2_T3_BOTTOM_LEFT_RIGHT, T2_T3_TOP_RIGHT_BOTTOM_LEFT_RIGHT,
				T2_T3_TOP_LEFT_BOTTOM_RIGHT, T2_T3_TOP_LEFT_RIGHT_BOTTOM_RIGHT,	T2_T3_TOP_LEFT_BOTTOM_LEFT_RIGHT, T3_NONE,
				T1_T3_BOTTOM_RIGHT, T1_T3_TOP_RIGHT_BOTTOM_RIGHT,	T1_T3_BOTTOM_LEFT_RIGHT, T1_T3_TOP_RIGHT_BOTTOM_LEFT_RIGHT,
				T1_T3_TOP_LEFT_BOTTOM_RIGHT, T1_T3_TOP_LEFT_RIGHT_BOTTOM_RIGHT,	T1_T3_TOP_LEFT_BOTTOM_LEFT_RIGHT,
				//3 corner transitions
				T1_T2_TOP_RIGHT_T3_BOTTOM_RIGHT, T1_T2_BOTTOM_LEFT_T3_BOTTOM_RIGHT, T1_T2_TOP_RIGHT_BOTTOM_LEFT_T3_BOTTOM_RIGHT, T1_T2_BOTTOM_LEFT_T3_TOP_RIGHT_BOTTOM_RIGHT,
				T1_T2_TOP_RIGHT_T3_BOTTOM_LEFT_RIGHT, T1_T2_TOP_LEFT_T3_BOTTOM_RIGHT, T1_T2_TOP_LEFT_RIGHT_T3_BOTTOM_RIGHT,	T1_T2_TOP_LEFT_T3_TOP_RIGHT_BOTTOM_RIGHT,
				T1_T2_TOP_LEFT_BOTTOM_LEFT_T3_BOTTOM_RIGHT,	T1_T2_TOP_LEFT_T3_BOTTOM_LEFT_RIGHT, T1_T2_TOP_RIGHT_T3_TOP_LEFT_BOTTOM_RIGHT, T1_T2_BOTTOM_LEFT_T3_TOP_LEFT_BOTTOM_RIGHT };
		}

		static const std::array<Types, 16> terrain1_to_terrain2 = { T1_NONE, T1_T2_TOP_RIGHT, T1_T2_BOTTOM_RIGHT, T1_T2_TOP_RIGHT_BOTTOM_RIGHT, T1_T2_BOTTOM_LEFT,
			T1_T2_TOP_RIGHT_BOTTOM_LEFT, T1_T2_BOTTOM_LEFT_RIGHT, T1_T2_TOP_RIGHT_BOTTOM_LEFT_RIGHT, T1_T2_TOP_LEFT,
			T1_T2_TOP_LEFT_RIGHT, T1_T2_TOP_LEFT_BOTTOM_RIGHT, T1_T2_TOP_LEFT_RIGHT_BOTTOM_RIGHT, T1_T2_TOP_LEFT_BOTTOM_LEFT,
			T1_T2_TOP_LEFT_RIGHT_BOTTOM_LEFT, T1_T2_TOP_LEFT_BOTTOM_LEFT_RIGHT, T2_NONE };

		//NOTE: this is equivalent to terrain1_to_terrain2 + 40;
		static const std::array<Types, 16> terrain2_to_terrain3 = { T2_NONE, T2_T3_TOP_RIGHT, T2_T3_BOTTOM_RIGHT, T2_T3_TOP_RIGHT_BOTTOM_RIGHT, T2_T3_BOTTOM_LEFT,
			T2_T3_TOP_RIGHT_BOTTOM_LEFT, T2_T3_BOTTOM_LEFT_RIGHT, T2_T3_TOP_RIGHT_BOTTOM_LEFT_RIGHT, T2_T3_TOP_LEFT,
			T2_T3_TOP_LEFT_RIGHT, T2_T3_TOP_LEFT_BOTTOM_RIGHT, T2_T3_TOP_LEFT_RIGHT_BOTTOM_RIGHT, T2_T3_TOP_LEFT_BOTTOM_LEFT,
			T2_T3_TOP_LEFT_RIGHT_BOTTOM_LEFT, T2_T3_TOP_LEFT_BOTTOM_LEFT_RIGHT, T3_NONE };

		//NOTE: this is equivalent to terrain1_to_terrain2 * 2;
		static const std::array<Types, 16> terrain1_to_terrain3 = { T1_NONE, T1_T3_TOP_RIGHT, T1_T3_BOTTOM_RIGHT, T1_T3_TOP_RIGHT_BOTTOM_RIGHT, T1_T3_BOTTOM_LEFT,
			T1_T3_TOP_RIGHT_BOTTOM_LEFT, T1_T3_BOTTOM_LEFT_RIGHT, T1_T3_TOP_RIGHT_BOTTOM_LEFT_RIGHT, T1_T3_TOP_LEFT,
			T1_T3_TOP_LEFT_RIGHT, T1_T3_TOP_LEFT_BOTTOM_RIGHT, T1_T3_TOP_LEFT_RIGHT_BOTTOM_RIGHT, T1_T3_TOP_LEFT_BOTTOM_LEFT,
			T1_T3_TOP_LEFT_RIGHT_BOTTOM_LEFT, T1_T3_TOP_LEFT_BOTTOM_LEFT_RIGHT, T3_NONE };


		tile PickTile3Corner(const std::array<hades::data::UniqueId, 4> &corners)
		{
			std::set<hades::data::UniqueId> unique_terrain(corners.begin(), corners.end());
			unique_terrain.erase(hades::data::UniqueId::Zero);
			unique_terrain.erase(GetErrorTerrain());

			if (unique_terrain.size() != 3)
				return transition2::PickTile2Corner(corners);

			auto iter = unique_terrain.begin();

			auto terrain1 = *iter++,
				terrain2 = *iter++,
				terrain3 = *iter;

			//pick a transition that contains the three terrains
			ortho_terrain::resources::terrain_transition3 *transition = nullptr;
			const std::array<hades::data::UniqueId, 3> terrain_group = { terrain1, terrain2, terrain3 };

			auto match = [&terrain_group](resources::terrain_transition3 *transition)->bool {
				const std::array<hades::data::UniqueId, 3> terrains =
				{ transition->terrain1, transition->terrain2, transition->terrain3 };

				hades::types::uint8 count = 0;
				for (auto &t : terrain_group)
				{
					for (auto &t2 : terrains)
					{
						if (t == t2)
						{
							count++;
							break;
						}
					}
				}

				return count == 3;
			};

			auto terrain1_obj = hades::data_manager->get<resources::terrain>(terrain1);
			for (auto &transid : terrain1_obj->transitions3)
			{
				auto trans = hades::data_manager->get<resources::terrain_transition3>(transid);
				if (match(trans))
				{
					transition = trans;
					break;
				}
			}

			hades::types::uint8 bitIndex = 0;

			//top right corner
			if (corners[Corner::TOPRIGHT] == transition->terrain2)
				bitIndex += 1;
			else if (corners[Corner::TOPRIGHT] == transition->terrain3)
				bitIndex += 2;

			//bottom right
			if (corners[Corner::BOTTOMRIGHT] == transition->terrain2)
				bitIndex += 3;
			else if (corners[Corner::BOTTOMRIGHT] == transition->terrain3)
				bitIndex += 6;

			//bottom left
			if (corners[Corner::BOTTOMLEFT] == transition->terrain2)
				bitIndex += 9;
			else if (corners[Corner::BOTTOMLEFT] == transition->terrain3)
				bitIndex += 18;

			//top left
			if (corners[Corner::TOPLEFT] == transition->terrain2)
				bitIndex += 27;
			else if (corners[Corner::TOPLEFT] == transition->terrain3)
				bitIndex += 54;

			auto &tiles = GetTransition(static_cast<transition3::Types>(bitIndex), *transition);
			return RandomTile(tiles);
		}

		hades::data::UniqueId TerrainInCorner3(const tile& tile, Corner corner)
		{
			using namespace transition3::transition_corners;

			auto t = GetTerrainInfo(tile);

			switch (corner)
			{
			case TOPRIGHT:
				if (std::find(terrain2_top_right.begin(), terrain2_top_right.end(), t.type3) != std::end(terrain2_top_right))
					return t.terrain2;
				else if (std::find(terrain3_top_right.begin(), terrain3_top_right.end(), t.type3) != std::end(terrain3_top_right))
					return t.terrain3;
				else
					return t.terrain;
			case BOTTOMRIGHT:
				if (std::find(terrain2_bottom_right.begin(), terrain2_bottom_right.end(), t.type3) != std::end(terrain2_bottom_right))
					return t.terrain2;
				else if (std::find(terrain3_bottom_right.begin(), terrain3_bottom_right.end(), t.type3) != std::end(terrain3_bottom_right))
					return t.terrain3;
				else
					return t.terrain;
			case BOTTOMLEFT:
				if (std::find(terrain2_bottom_left.begin(), terrain2_bottom_left.end(), t.type3) != std::end(terrain2_bottom_left))
					return t.terrain2;
				else if (std::find(terrain3_bottom_left.begin(), terrain3_bottom_left.end(), t.type3) != std::end(terrain3_bottom_left))
					return t.terrain3;
				else
					return t.terrain;
			case TOPLEFT:
				if (std::find(terrain2_top_left.begin(), terrain2_top_left.end(), t.type3) != std::end(terrain2_top_left))
					return t.terrain2;
				else if (std::find(terrain3_top_left.begin(), terrain3_top_left.end(), t.type3) != std::end(terrain3_top_left))
					return t.terrain3;
				else
					return t.terrain;
			default:
				return hades::data::UniqueId::Zero;
			}
		}
	}

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

	static const auto CONVERT_ERROR = -1;

	hades::types::int32 ConvertToTransition2(transition3::Types t)
	{
		for (std::array<transition3::Types, 16>::size_type i = 0; i < transition3::terrain1_to_terrain2.size(); ++i)
		{
			auto val = transition3::terrain1_to_terrain2[i];
			if (val	== t ||
				val + 40 == t ||
				val * 2 == t)
				return i;
		}

		//TODO: throw exception here instead,
		//this error represents a configuration or programming error
		//and is not performace critical
		return CONVERT_ERROR;
	}

	enum class ConvertSetting { TERRAIN1_2, TERRAIN2_3, TERRAIN1_3, NO_CONVERT };

	ConvertSetting PickSetting(transition3::Types type)
	{
		auto type_int = static_cast<hades::types::int8>(type);
		auto less_40 = type_int - 40;
		if (less_40 < 0)
			less_40 = 81;
		auto half = type_int / 2;
		if (type_int % 2 != 0)
			half = 81;

		for (const auto &t : transition3::terrain1_to_terrain2)
		{
			if (t == type_int)
				return ConvertSetting::TERRAIN1_2;
			else if (t == less_40)
				return ConvertSetting::TERRAIN2_3;
			else if (t == half)
				return ConvertSetting::TERRAIN1_3;
		}

		return ConvertSetting::NO_CONVERT;
	}

	transition3::Types ConvertToTransition3(transition2::TransitionTypes t, const resources::terrain_transition3& transition, ConvertSetting setting)
	{
		auto type = static_cast<hades::types::uint8>(transition3::terrain1_to_terrain2[t]);

		//See: http://www.cr31.co.uk/stagecast/wang/3corn.html
		//The Terrain2 & Terrain3 tileset has an index of + 40 Terrain1 & Terrain2.
		//the Terrain1 & Terrain3 tileset has an index of twice Terrain1 & Terrain2.
		if (setting == ConvertSetting::TERRAIN2_3)
			type += 40;
		else if (setting == ConvertSetting::TERRAIN1_3)
			type *= 2;

		return static_cast<transition3::Types>(type);
	}

	template<class U = std::vector<tile>, class V = resources::terrain_transition>
	U& GetTransition(transition2::TransitionTypes type, V& transitions)
	{
		using namespace transition2;

		switch (type)
		{
		case NONE:
		{
			auto terrain1 = hades::data_manager->get<resources::terrain>(transitions.terrain1);
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
			auto terrain2 = hades::data_manager->get<resources::terrain>(transitions.terrain2);
			return terrain2->tiles;
		}
		default:
			LOGWARNING("'Type' passed to GetTransition was outside expected range[0,15] was: " + std::to_string(type));
			
			return tiles::GetErrorTileset();
		}
	}

	std::vector<tile>& GetTransition(transition2::TransitionTypes type, resources::terrain_transition& transition)
	{
		return GetTransition<std::vector<tile>, resources::terrain_transition>(type, transition);
	}

	const std::vector<tile>& GetTransition(transition2::TransitionTypes type, const resources::terrain_transition& transition)
	{
		return GetTransition<const std::vector<tile>, const resources::terrain_transition>(type, transition);
	}

	template<class U = std::vector<tile>, class V = resources::terrain_transition3>
	U& GetTransition(transition3::Types type, V& transitions)
	{
		using namespace transition3;
		//check 3 tile interactions
		switch (type)
		{
			//first 12
		case T1_T2_BOTTOM_RIGHT_T3_TOP_RIGHT:
			return transitions.t2_bottom_right_t3_top_right;
		case T1_T2_TOP_RIGHT_T3_BOTTOM_RIGHT:
			return transitions.t2_top_right_t3_bottom_right;
		case T1_T2_BOTTOM_LEFT_T3_TOP_RIGHT:
			return transitions.t2_bottom_left_t3_top_right;
		case T1_T2_BOTTOM_LEFT_RIGHT_T3_TOP_RIGHT:
			return transitions.t2_bottom_left_right_t3_top_right;
		case T1_T2_BOTTOM_LEFT_T3_BOTTOM_RIGHT:
			return transitions.t2_bottom_left_t3_bottom_right;
		case T1_T2_TOP_RIGHT_BOTTOM_LEFT_T3_BOTTOM_RIGHT:
			return transitions.t2_top_right_bottom_left_t3_bottom_right;
		case T1_T2_BOTTOM_LEFT_T3_TOP_RIGHT_BOTTOM_RIGHT:
			return transitions.t2_bottom_left_t3_top_right_bottom_right;
		case T1_T2_TOP_RIGHT_T3_BOTTOM_LEFT:
			return transitions.t2_top_right_t3_bottom_left;
		case T1_T2_BOTTOM_RIGHT_T3_BOTTOM_LEFT:
			return transitions.t2_bottom_right_t3_bottom_left;
		case T1_T2_TOP_RIGHT_BOTTOM_RIGHT_T3_BOTTOM_LEFT:
			return transitions.t2_top_right_bottom_right_t3_bottom_left;
		case T1_T2_BOTTOM_RIGHT_T3_TOP_RIGHT_BOTTOM_LEFT:
			return transitions.t2_bottom_right_t3_top_right_bottom_left;
		case T1_T2_TOP_RIGHT_T3_BOTTOM_LEFT_RIGHT:
			return transitions.t2_top_right_t3_bottom_left_right;
			//second 12
		case T1_T2_TOP_LEFT_T3_TOP_RIGHT:
			return transitions.t2_top_left_t3_top_right;
		case T1_T2_TOP_LEFT_BOTTOM_RIGHT_T3_TOP_RIGHT:
			return transitions.t2_top_left_bottom_right_t3_top_right;
		case T1_T2_TOP_LEFT_T3_BOTTOM_RIGHT:
			return transitions.t2_top_left_t3_bottom_right;
		case T1_T2_TOP_LEFT_RIGHT_T3_BOTTOM_RIGHT:
			return transitions.t2_top_left_right_t3_bottom_right;
		case T1_T2_TOP_LEFT_T3_TOP_RIGHT_BOTTOM_RIGHT:
			return transitions.t2_top_left_t3_top_right_bottom_right;
		case T1_T2_TOP_LEFT_BOTTOM_LEFT_T3_TOP_RIGHT:
			return transitions.t2_top_left_bottom_left_t3_top_right;
		case T1_T2_TOP_LEFT_BOTTOM_LEFT_T3_BOTTOM_RIGHT:
			return transitions.t2_top_left_bottom_left_t3_bottom_right;
		case T1_T2_TOP_LEFT_T3_BOTTOM_LEFT:
			return transitions.t2_top_left_t3_bottom_left;
		case T1_T2_TOP_LEFT_RIGHT_T3_BOTTOM_LEFT:
			return transitions.t2_top_left_right_t3_bottom_left;
		case T1_T2_TOP_LEFT_T3_TOP_RIGHT_BOTTOM_LEFT:
			return transitions.t2_top_left_t3_top_right_bottom_left;
		case T1_T2_TOP_LEFT_BOTTOM_RIGHT_T3_BOTTOM_LEFT:
			return transitions.t2_top_left_bottom_right_t3_bottom_left;
		case T1_T2_TOP_LEFT_T3_BOTTOM_LEFT_RIGHT:
			return transitions.t2_top_left_t3_bottom_left_right;
			//final 12
		case T1_T2_TOP_RIGHT_T3_TOP_LEFT:
			return transitions.t2_top_right_t3_top_left;
		case T1_T2_BOTTOM_RIGHT_T3_TOP_LEFT:
			return transitions.t2_bottom_right_t3_top_left;
		case T1_T2_TOP_RIGHT_BOTTOM_RIGHT_T3_TOP_LEFT:
			return transitions.t2_top_right_bottom_right_t3_top_left;
		case T1_T2_BOTTOM_RIGHT_T3_TOP_LEFT_RIGHT:
			return transitions.t2_bottom_right_t3_top_left_right;
		case T1_T2_TOP_RIGHT_T3_TOP_LEFT_BOTTOM_RIGHT:
			return transitions.t2_top_right_t3_top_left_bottom_right;
		case T1_T2_BOTTOM_LEFT_T3_TOP_LEFT:
			return transitions.t2_bottom_left_t3_top_left;
		case T1_T2_TOP_RIGHT_BOTTOM_LEFT_T3_TOP_LEFT:
			return transitions.t2_top_right_bottom_left_t3_top_left;
		case T1_T2_BOTTOM_LEFT_T3_TOP_LEFT_RIGHT:
			return transitions.t2_bottom_left_t3_top_left_right;
		case T1_T2_BOTTOM_LEFT_RIGHT_T3_TOP_LEFT:
			return transitions.t2_bottom_left_right_t3_top_left;
		case T1_T2_BOTTOM_LEFT_T3_TOP_LEFT_BOTTOM_RIGHT:
			return transitions.t2_bottom_left_t3_top_left_bottom_right;
		case T1_T2_TOP_RIGHT_T3_TOP_LEFT_BOTTOM_LEFT:
			return transitions.t2_top_right_t3_top_left_bottom_left;
		case T1_T2_BOTTOM_RIGHT_T3_TOP_LEFT_BOTTOM_LEFT:
			return transitions.t2_bottom_right_t3_top_left_bottom_left;
			//if no 3 tile interaction fits, grab a 2 tile interaction
		default:
			//find out which 2 tile interaction to use
			auto setting = PickSetting(type);
			auto type_int = ConvertToTransition2(type);
			if (type_int < 0)
				setting = ConvertSetting::NO_CONVERT;

			auto type2 = static_cast<transition2::TransitionTypes>(type_int);

			if (setting == ConvertSetting::TERRAIN1_2)
			{
				auto trans = hades::data_manager->get<resources::terrain_transition>(transitions.transition_1_2);
				return GetTransition(type2, *trans);
			}
			else if (setting == ConvertSetting::TERRAIN2_3)
			{
				auto trans = hades::data_manager->get<resources::terrain_transition>(transitions.transition_2_3);
				return GetTransition(type2, *trans);
			}
			else if (setting == ConvertSetting::TERRAIN1_3)
			{
				auto trans = hades::data_manager->get<resources::terrain_transition>(transitions.transition_1_3);
				return GetTransition(type2, *trans);
			}
			else
			{
				LOGWARNING("'Type' passed to GetTransition was outside expected range[0,80] was: " + std::to_string(type));
				return tiles::GetErrorTileset();
			}
		};
	}

	std::vector<tile>& GetTransition(transition3::Types type, resources::terrain_transition3& transition)
	{
		return GetTransition<std::vector<tile>, resources::terrain_transition3>(type, transition);
	}

	const std::vector<tile>& GetTransition(transition3::Types type, const resources::terrain_transition3& transition)
	{
		return GetTransition<const std::vector<tile>, const resources::terrain_transition3>(type, transition);
	}
}
