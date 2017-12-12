#include "OrthoTerrain/terrain.hpp"

#include <array>

#include "SFML/Graphics/RenderTarget.hpp"

#include "Hades/Data.hpp"
#include "Hades/DataManager.hpp"
#include "Hades/data_manager.hpp"
#include "Hades/Utility.hpp"

#include "OrthoTerrain/utility.hpp"
#include "OrthoTerrain/resources.hpp"

#include "Tiles/tiles.hpp"

namespace ortho_terrain
{
	namespace
	{
		static const tiles::VertexArray::size_type VertexPerTile = 6;
	}

	using tiles::MapData;
	using tiles::tile_count_t;
	using tiles::TileArray;
	using tiles::tile;

	MapData ConvertToTiles(const TerrainVertex&, tile_count_t vertexPerRow)
	{
		//TODO:
		//go tile by tile generating transitions
		return std::make_tuple<TileArray, tile_size_t>(TileArray(), 0);
	}

	VertexMapData ConvertToVertex(const MapData& map)
	{
		auto &tiles = std::get<TileArray>(map);
		auto tilesPerRow = std::get<tile_count_t>(map);
		TerrainVertex out;
		tile_count_t count = 0;

		//add the topleft of each tile
		for (auto &t : tiles)
		{
			//TODO: error handling
			out.push_back(hades::data::Get<resources::terrain>(TerrainInCorner(t, Corner::TOPLEFT)));

			if (count++ == tilesPerRow)
			{
				out.push_back(hades::data::Get<resources::terrain>(TerrainInCorner(t, Corner::TOPRIGHT)));
				count = 0;
			}
		}

		//add the bottoms of the last row
		auto iterator = tiles.cend() - tilesPerRow;
		for (iterator; iterator != tiles.cend(); ++iterator)
			out.push_back(hades::data::Get<resources::terrain>(TerrainInCorner(*iterator, Corner::BOTTOMLEFT)));

		out.push_back(hades::data::Get<resources::terrain>(TerrainInCorner(*tiles.crbegin(), Corner::BOTTOMLEFT)));

		return std::make_tuple(out, ++tilesPerRow);
	}

	MutableTerrainMap::MutableTerrainMap(const MapData &map)
	{
		create(map);
	}

	void MutableTerrainMap::create(const MapData &map_data)
	{
		//create the vertex map
		auto vert = ConvertToVertex(map_data);
		_vertex = std::get<TerrainVertex>(vert);
		_vertex_width = std::get<tile_count_t>(vert);

		MutableTileMap::create(map_data);
	}

	std::vector<sf::Vector2i> AllPositions(const sf::Vector2u &position, hades::types::uint8 amount)
	{
		if (amount == 0)
			return { sf::Vector2i(position) };

		auto end = sf::Vector2i(position) + 
			static_cast<sf::Vector2i>(sf::Vector2f{ std::floorf(amount / 2.f), std::floorf(amount / 2.f) });
		auto start_position = sf::Vector2i(position) -
			static_cast<sf::Vector2i>(sf::Vector2f{ std::ceilf(amount / 2.f), std::ceilf(amount / 2.f)});

		std::vector<sf::Vector2i> positions;

		for (auto r = start_position.x; r < end.x; ++r)
		{
			for (auto c = start_position.y; c < end.y; ++c)
				positions.push_back({ r, c });
		}

		return positions;
	}

	//returns the 4 vertex in the corners of the tile position
	//[0] top left
	//[1] top right
	//[2] bottom left
	//[3] bottom right
	std::array<sf::Vector2u, 4> AsVertex(const sf::Vector2u &position)
	{
		//the vertex array always have one extra colomn and row,
		//so adding to the tile position never goes out of bounds
		std::array<sf::Vector2u, 4> vert;
		vert[Corner::TOPLEFT] = position;
		vert[Corner::TOPRIGHT] = { position.x + 1, position.y };
		vert[Corner::BOTTOMLEFT] = { position.x, position.y + 1 };
		vert[Corner::BOTTOMRIGHT] = {position.x + 1, position.y + 1};

		return vert;
	}

	//returns the 4 tiles that touch this vertex
	//[0] top left
	//[1] top right
	//[2] bottom left
	//[3] bottom right
	std::array<sf::Vector2i, 4> AsTiles(const sf::Vector2u &position)
	{
		//if the tile is along the top or left edge of the map, 
		//then the positions would wrap around to the opposite extremities
		auto pos = static_cast<sf::Vector2i>(position);
		std::array<sf::Vector2i, 4> tiles;
		tiles[Corner::TOPLEFT] = { pos.x - 1, pos.y - 1 };
		tiles[Corner::TOPRIGHT] = { pos.x, pos.y - 1 };
		tiles[Corner::BOTTOMLEFT] = { pos.x - 1, pos.y };
		tiles[Corner::BOTTOMRIGHT] = pos;

		return tiles;
	}

	tile_count_t FlatPosition(const sf::Vector2u &position, tile_count_t width)
	{
		return position.y * width + position.x;
	}

	template<class T>
	bool IsWithin(std::vector<T> &dest, const sf::Vector2u &position, tile_count_t width)
	{
		auto pos = FlatPosition(position, width);
		return pos < dest.size() &&
			position.x < width;
	}

	//returns true if the value in dest was changed
	template<class T>
	bool Write(std::vector<T> &dest, const sf::Vector2u &position, tile_count_t width, const T &value)
	{
		auto pos = FlatPosition(position, width);
		auto changed = dest[pos] != value;
		dest[pos] = value;
		return changed;
	}

	void MutableTerrainMap::replace(const tile& t, const sf::Vector2u &position, hades::types::uint8 amount, bool updateVertex)
	{	
		MutableTileMap::replace(t, position, amount);

		if (!updateVertex)
			return;

		auto positions = AllPositions(position, amount);
		for (const auto &p : positions)
		{
			auto position = sf::Vector2u(p);
			//assert that the size of the corners enum hasnt been changed
			assert(CORNER_LAST == 4);

			auto verts = AsVertex(position);
			for(auto i = 0; i < CORNER_LAST; ++i)
			{
				if (!IsWithin(_vertex, verts[i], _vertex_width))
					continue;

				//get terrain
				//assert that i is in the correct range
				//for accessing the 4 element array
				assert(i >= 0 && i < 4);
				auto t_id = TerrainInCorner(t, static_cast<Corner>(i));
				//convert to resource
				const auto *terrain = hades::data::Get<resources::terrain>(t_id);
				//write to vertex
				Write(_vertex, verts[i], _vertex_width, terrain);
			}
		}
	}

	void MutableTerrainMap::replace(const resources::terrain& terrain, const sf::Vector2u &position, hades::types::uint8 amount)
	{
		auto positions = AllPositions(position, amount);

		std::vector<sf::Vector2u> changedVertex;
		//write over all vertex
		for (auto &p : positions)
		{
			auto position = sf::Vector2u(p);

			//tried to access a vertex outside the map
			if (!IsWithin(_vertex, position, _vertex_width))
				continue;

			if (Write(_vertex, position, _vertex_width, &terrain))
				changedVertex.push_back(position);
		}

		if (changedVertex.empty())
			return;

		//calculate the total changed area and update the tiles.
		tile_count_t smallest_x = _vertex_width, largest_x = 0,
			smallest_y = std::numeric_limits<tile_count_t>::max(), largest_y = 0;
		for (auto &v : changedVertex)
		{
			smallest_x = std::min(v.x, smallest_x);
			largest_x = std::max(v.x, largest_x);
			smallest_y = std::min(v.y, smallest_y);
			largest_y = std::max(v.y, largest_y);
		}

		_cleanTransitions({ smallest_x, smallest_y }, { largest_x - smallest_x, largest_y - smallest_y });
	}
	
	void MutableTerrainMap::_cleanTransitions(const sf::Vector2u &position, sf::Vector2u size)
	{
		//for each vertex in the area
		std::vector<sf::Vector2u> vert_list;
		for (auto r = position.x; r <= position.x + size.x; ++r)
		{
			for (auto c = position.y; c <= position.y + size.y; ++c)
			{
				vert_list.push_back({ r, c });
			}
		}

		auto less = [](auto a, auto b) { if (a.x == b.x) return a.y < b.y; else return a.x < b.x; };
		std::set<sf::Vector2i, decltype(less)> work_list(less);
		for (auto &pos : vert_list)
		{
			auto tiles = AsTiles(pos);
			work_list.insert(tiles.begin(), tiles.end());
		}

		for (auto &pos : work_list)
		{
			//skip tiles outside the map bounds
			if (pos.x < 0 || pos.y < 0)
				continue;

			//get the 4 corners
			auto verts = AsVertex(static_cast<sf::Vector2u>(pos));
			std::array<const resources::terrain*, 4> corners;
			for (std::size_t i = 0; i < verts.size(); ++i)
			{
				if (IsWithin(_vertex, verts[i], _vertex_width))
				{
					auto pos = FlatPosition(verts[i], _vertex_width);
					corners[i] = _vertex[pos];
				}
				else
					corners[i] = nullptr;
			}

			auto tile = PickTile(corners);
			replace(tile, static_cast<sf::Vector2u>(pos));
		}
	}

	const resources::terrain_settings &GetTerrainSettings()
	{
		auto settings_id = hades::data::GetUid(resources::terrain_settings_name);
		if (!hades::data::Exists(settings_id))
		{
			auto message = "terrain-settings undefined.";
			LOGERROR(message)
				throw exception(message);
		}

		try
		{
			return *hades::data::Get<resources::terrain_settings>(settings_id);
		}
		catch (hades::data::resource_wrong_type&)
		{
			auto message = "The UID for the tile settings has been reused for another resource type";
			LOGERROR(message);
			throw exception(message);
		}
	}

	hades::data::UniqueId GetErrorTerrain()
	{
		auto settings = GetTerrainSettings();
		return settings.error_terrain;
	}


	tiles::traits_list GetTerrainTraits(const tiles::tile &tile)
	{
		const auto terrain = GetTerrainInfo(tile);
		auto out = tile.traits;
		const auto terrain_list = { terrain.terrain, terrain.terrain2, terrain.terrain3, terrain.terrain4 };

		for (auto &t_id : terrain_list)
		{
			if (t_id == hades::data::UniqueId::Zero ||
				!hades::data::Exists(t_id))
				continue;

			auto t = hades::data::Get<resources::terrain>(t_id);
			std::copy(std::begin(t->traits), std::end(t->traits), std::back_inserter(out));
		}

		return out;
	}

	terrain_info GetTerrainInfo(const tiles::tile &tile)
	{
		auto it = TerrainLookup.find(tile);
		if (it == std::end(TerrainLookup))
			return terrain_info();

		return it->second;
	}
}
