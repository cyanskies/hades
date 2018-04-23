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

	std::size_t VertWidth(std::size_t tiles_width)
	{
		return ++tiles_width;
	}

	TerrainVertex AsTerrainVertex(const std::vector<tiles::TileArray> &map, const std::vector<const resources::terrain*> &terrainset, tiles::tile_count_t width)
	{
		assert(!map.empty());
		if (map[0].size() % width != 0)
			throw exception("TileMap is not rectangular, map width: " + hades::to_string(width) + ", tile_count: " + hades::to_string(map[0].size()));

		const auto coloumns = map[0].size() / width;

		static const auto empty_terrain = hades::data::Get<resources::terrain>(resources::EmptyTerrainId);

		//a vert array includes a single extra column and row
		const auto vert_width = VertWidth(width);
		const auto vert_length = (coloumns + 1) * vert_width;
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

	bool Within(sf::Vector2i pos, std::size_t target_size, tiles::tile_count_t width)
	{
		const auto flat_pos = tiles::FlatPosition(static_cast<sf::Vector2u>(pos), width);
		return flat_pos < target_size &&
			pos.x < width;
	}

	void ReplaceTerrain(TerrainMapData &map, TerrainVertex &verts, tiles::tile_count_t width, const resources::terrain *t, sf::Vector2i pos, tiles::draw_size_t size)
	{
		if (map.terrain_set.size() != map.tile_map_stack.size())
			throw exception("Map data malformed, should contain one tile layer per terrainset entry");

		const auto positions = tiles::AllPositions(pos, size);
		auto &tile_layers = map.tile_map_stack;
		
		const auto vert_width = VertWidth(width);
		//for each position, update the vertex
		for (const auto &pos : positions)
		{
			if (Within(pos, verts.size(), vert_width))
			{
				const auto f_pos = tiles::FlatPosition(static_cast<sf::Vector2u>(pos), vert_width);
				verts[f_pos] = t;
			}
		}

		static const auto empty_terrain = hades::data::Get<resources::terrain>(resources::EmptyTerrainId);

		//we only want to update tiles within reach of the vertex changes
		const auto tile_positions = tiles::AllPositions(pos, size + 1);
		//then update the tiles based on the new vertex map
		for (std::size_t i = 0; i < tile_layers.size(); ++i)
		{
			auto &layer = tile_layers[i];
			const auto terrain = map.terrain_set[i];
			const auto layer_size = layer.size();
			for (const auto &pos : tile_positions)
			{
				if (!Within(pos, layer.size(), width))
					continue;

				const auto corners = GetCornerData(static_cast<sf::Vector2u>(pos), verts, vert_width);
				std::array<bool, 4> empty_corners{ false };

				for (std::size_t k = 0; k < corners.size(); ++k)
					empty_corners[k] = corners[k] == empty_terrain ? true : false;

				const auto type = PickTransition(empty_corners);
				const auto tile_list = GetTransitionConst(type, *terrain);
				const auto tile = RandomTile(tile_list);
				const auto flat_pos = tiles::FlatPosition(static_cast<sf::Vector2u>(pos), width);
				layer[flat_pos] = tile;
			}
		}
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
	
	sf::FloatRect GetBounds(const std::vector<const tiles::TileMap*> &map)
	{
		if (map.empty())
			return sf::FloatRect{};
		else
		{
			sf::FloatRect r{};
			for (const auto &l : map)
			{
				const auto rect = l->getLocalBounds();
				r.left = std::min(r.left, rect.left);
				r.top = std::min(r.top, rect.top);
				r.width = std::max(r.width, rect.width);
				r.height = std::max(r.height, rect.height);
			}

			return r;
		}
	}

	sf::FloatRect TerrainMap::getLocalBounds() const
	{
		std::vector<const tiles::TileMap*> map;
		for (const auto &layer : Map)
			map.emplace_back(&layer);

		return GetBounds(map);
	}

	///////////////////////
	// MutableTerrainMap //
	///////////////////////

	MutableTerrainMap::MutableTerrainMap(const TerrainMapData &dat, tiles::tile_count_t width)
	{
		create(dat, width);
	}

	void MutableTerrainMap::create(const TerrainMapData &map, tiles::tile_count_t width)
	{
		tiles::tile_count_t size = 0u;
		for (const auto &layer : map.tile_map_stack)
			size = std::max(size, layer.size());

		std::vector<tile_layer> new_map;

		if (map.tile_map_stack.size() > map.terrain_set.size())
			throw exception("terrain map is malformed, contains more tile layers than terrain types");
		
		//ensure the map is well formed, and patch it up if needed
		const auto empty_tile = tiles::GetEmptyTile();
		const auto &empty_terrain = resources::EmptyTerrainId;
		tiles::TileArray empty_layer{ size, empty_tile };
		std::size_t i = 0u, j = 0u;
		while (j < map.terrain_set.size())
		{
			const auto &layer = i > map.tile_map_stack.size() ? map.tile_map_stack[i] : empty_layer;
			const auto terrain = FindTerrain(layer, map.terrain_set);
			if (terrain->id == empty_terrain)
				continue;

			if (terrain != map.terrain_set[i])
			{
				new_map.emplace_back(map.terrain_set[i], empty_layer);
				++i;
			}
			else
			{
				new_map.emplace_back(terrain, layer);
				++i; ++j;
			}
		}

		std::vector<tiles::TileArray> tile_array;
		for (const auto &arr : new_map)
			tile_array.emplace_back(std::get<tiles::TileArray>(arr));

		//generate vertex map
		auto verts = AsTerrainVertex(tile_array, map.terrain_set, width);

		for (const auto &tiles : new_map)
			_terrain_layers.emplace_back(tiles::MutableTileMap{ {std::get<tiles::TileArray>(tiles), width} });

		std::swap(new_map, _tile_layers);
		std::swap(verts, _vdata);
		_width = width;

		//apply colour if it's already been set
		setColour(_colour);
	}

	void MutableTerrainMap::create(std::vector<const resources::terrain*> terrainset, tiles::tile_count_t width, tiles::tile_count_t height)
	{
		const auto size = width * height;

		//ensure the map is well formed, and patch it up if needed
		const auto empty_tile = tiles::GetEmptyTile();
		const auto &empty_terrain = resources::EmptyTerrainId;
		tiles::TileArray empty_layer{ size, empty_tile };
		const auto bottom_terrain = terrainset.front();
		tiles::TileArray bottom_layer{ size, bottom_terrain->full.front() };

		_width = width;
		std::vector<tiles::TileArray> t_array;

		for (const auto &t : terrainset)
		{
			if (t == bottom_terrain)
			{
				_tile_layers.emplace_back(t, bottom_layer);
				t_array.emplace_back(bottom_layer);
			}
			else
			{
				_tile_layers.emplace_back(t, empty_layer);
				t_array.emplace_back(empty_layer);
			}
		}

		//generate vertex map
		_vdata = AsTerrainVertex(t_array, terrainset, width);

		for (const auto &tiles : t_array)
			_terrain_layers.emplace_back(tiles::MutableTileMap{ {tiles, width} });

		//apply colour if it's already been set
		setColour(_colour);
	}

	void MutableTerrainMap::draw(sf::RenderTarget& target, sf::RenderStates states) const
	{	
		for (const auto &layer : _terrain_layers)
			layer.draw(target, states);
	}

	sf::FloatRect MutableTerrainMap::getLocalBounds() const
	{
		std::vector<const tiles::TileMap*> map;
		for (const auto &layer : _terrain_layers)
			map.emplace_back(&layer);

		return GetBounds(map);
	}

	void MutableTerrainMap::replace(const resources::terrain *t, const sf::Vector2i &position, tiles::draw_size_t amount)
	{
		TerrainMapData map = getMap();
		ReplaceTerrain(map, _vdata, _width, t, position, amount);

		for (std::size_t i = 0u; i < map.tile_map_stack.size(); ++i)
		{
			std::swap(std::get<tiles::TileArray>(_tile_layers[i]), map.tile_map_stack[i]);
			_terrain_layers[i].update({ map.tile_map_stack[i], _width });
		}
	}

	void MutableTerrainMap::setColour(sf::Color c)
	{
		_colour = c;

		for (auto &l : _terrain_layers)
			l.setColour(c);
	}

	TerrainMapData MutableTerrainMap::getMap() const
	{
		TerrainMapData map;
		for (const auto &[terrain, tiles] : _tile_layers)
		{
			map.terrain_set.emplace_back(terrain);
			map.tile_map_stack.emplace_back(tiles);
		}

		return map;
	}
}
