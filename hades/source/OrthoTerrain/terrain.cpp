#include "OrthoTerrain/terrain.hpp"

#include <array>

#include "SFML/Graphics/RenderTarget.hpp"

#include "Hades/Data.hpp"
#include "Hades/Utility.hpp"

#include "OrthoTerrain/utility.hpp"
#include "OrthoTerrain/resources.hpp"

#include "Tiles/tiles.hpp"

namespace ortho_terrain
{
	void EnableTerrain(hades::data::data_system *d)
	{
		tiles::EnableTiles(d);
		RegisterOrthoTerrainResources(d);
	}

	std::array<bool, 4> TerrainInCorner(tiles::tile t, const resources::terrain *ter)
	{
		assert(ter);
		const auto t_type = GetTransitionType(t, ter);
		//top left
		const auto tl = IsTerrainInCorner(Corner::TOP_LEFT, t_type);
		//top right
		const auto tr = IsTerrainInCorner(Corner::TOP_RIGHT, t_type);
		//bottom left
		const auto bl = IsTerrainInCorner(Corner::BOTTOM_LEFT, t_type);
		//bottom right
		const auto br = IsTerrainInCorner(Corner::BOTTOM_RIGHT, t_type);

		return { tl, tr, bl, br };
	}

	std::vector<const resources::terrain*> &CalculateVertexLayer(const tiles::TileArray &layer, TerrainVertex &out, const resources::terrain *terrain, tiles::tile_count_t width)
	{
		for (std::size_t i = 0u; i < layer.size(); ++i)
		{
			const auto corners = TerrainInCorner(layer[i], terrain);

			if (corners[0])
				out[i] = terrain;
			if (corners[1])
				out[i + 1] = terrain;
			if (corners[2])
				out[i + width] = terrain;
			if (corners[3])
				out[i + width + 1] = terrain;
		}

		return out;
	}

	TerrainVertex CalculateVertex(const std::vector<tiles::TileArray> &map, const std::vector<const resources::terrain*> &terrainset, tiles::tile_count_t width)
	{
		assert(!map.empty());
		if (map[0].size() % width != 0)
			throw exception("TileMap is not rectangular, map width: " + hades::to_string(width) + ", tile_count: " + hades::to_string(map[0].size()));

		const auto coloumns = map[0].size() / width;

		static const auto empty_terrain = hades::data::Get<resources::terrain>(resources::EmptyTerrainId);

		//a vert array includes a single extra column and row
		const auto vert_length = coloumns + 1 * width + 1;
		TerrainVertex verts{ vert_length, empty_terrain };
		assert(map.size() <= terrainset.size());
		for (std::size_t i = 0u; i < map.size(); ++i)
			CalculateVertexLayer(map[i], verts, terrainset[i], width);

		return verts;
	}

	//returns the terrain the tile is sourced from
	const resources::terrain *FindTerrain(tiles::tile t, const std::vector<const resources::terrain*> &terrain_set)
	{
		for (const auto &terr : terrain_set)
		{
			const std::array<const std::vector<tiles::tile>*, 15> tile_arrays = { &terr->full, &terr->bottom_left_corner, &terr->top_left_corner, &terr->left,
				&terr->top_right_corner, &terr->left_diagonal, &terr->top, &terr->top_left_circle, &terr->bottom_right_corner,
				&terr->bottom, &terr->right_diagonal, &terr->bottom_left_circle, &terr->right, &terr->bottom_right_circle, &terr->top_right_circle
			};

			for (const auto &t_arr : tile_arrays)
			{
				if (std::any_of(std::begin(*t_arr), std::end(*t_arr), [t](const auto &tile)
				{
					return t == tile;
				}))
					return terr;
			}
		}

		return nullptr;
	}

	//finds out which terrain the provided layer is for
	const resources::terrain *FindTerrain(const tiles::TileArray &layer, const std::vector<const resources::terrain*> &terrain_set)
	{
		//terrain set should not contain the empty terrain
		assert(std::none_of(std::begin(terrain_set), std::end(terrain_set), [](const auto &terrain) {
			return terrain->id == resources::EmptyTerrainId;
		}));

		for (const auto &t : layer)
		{
			if (const auto *terrain = FindTerrain(t, terrain_set); terrain)
				return terrain;
		}

		//throw unexpected terrain?
		return nullptr;
	}

	////////////////
	// TerrainMap //
	////////////////

	TerrainMap::TerrainMap(const TerrainMapData &dat, tiles::tile_count_t width)
	{
		create(dat, width);
	}

	void TerrainMap::create(const TerrainMapData &map, tiles::tile_count_t width)
	{
		if (map.terrain_set.size() != map.tile_map_stack.size())
			throw exception("Map malformed, should contain a layer for each terrain in terrainset");

		for (const auto &l : map.tile_map_stack)
			Map.emplace_back(tiles::TileMap({ l, width }));
	}

	void TerrainMap::draw(sf::RenderTarget& target, sf::RenderStates states) const
	{
		for (const auto &l : Map)
			l.draw(target, states);
	}

	sf::FloatRect TerrainMap::getLocalBounds() const
	{
		if (Map.empty())
			return sf::FloatRect{};
		else
		{
			sf::FloatRect r{};
			for (const auto &l : Map)
			{
				const auto rect = l.getLocalBounds();
				r.left = std::min(r.left, rect.left);
				r.top = std::min(r.top, rect.top);
				r.width = std::max(r.width, rect.width);
				r.height = std::max(r.height, rect.height);
			}

			return r;
		}
	}

	///////////////////////
	// MutableTerrainMap //
	///////////////////////

	void MutableTerrainMap::create(const TerrainMapData&, tiles::tile_count_t width)
	{

	}

	void MutableTerrainMap::replace(const resources::terrain *t, const sf::Vector2i &position, tiles::draw_size_t amount)
	{}

	void MutableTerrainMap::setColour(sf::Color c)
	{
		_colour = c;

		for (auto &l : _terrain_layers)
			std::get<tiles::MutableTileMap>(l).setColour(c);
	}

	TerrainMapData MutableTerrainMap::getMap() const
	{
		return TerrainMapData{};
	}
}
