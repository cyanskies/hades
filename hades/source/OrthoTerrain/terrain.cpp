#include "OrthoTerrain/terrain.hpp"

#include <array>

#include "SFML/Graphics/RenderTarget.hpp"

#include "Hades/data_manager.hpp"
#include "Hades/Utility.hpp"

#include "OrthoTerrain/utility.hpp"
#include "OrthoTerrain/resources.hpp"

#include "Tiles/tiles.hpp"

namespace ortho_terrain
{
	namespace
	{
		static const VertexArray::size_type VertexPerTile = 6;
	}

	MapData ConvertToTiles(const TerrainVertex&, tile_count_t vertexPerRow)
	{
		//TODO:
		//go tile by tile generating transitions
		return std::make_tuple<TileArray, tile_size_t>(TileArray(), 0);
	}

	VertexData ConvertToVertex(const MapData& map)
	{
		auto &tiles = std::get<TileArray>(map);
		auto tilesPerRow = std::get<tile_count_t>(map);
		TerrainVertex out;
		tile_count_t count = 0;

		//add the topleft of each tile
		for (auto &t : tiles)
		{
			//TODO: error handling
			out.push_back(hades::data_manager->get<resources::terrain>(TerrainInCorner(t, Corner::TOPLEFT)));

			if (count++ == tilesPerRow)
			{
				out.push_back(hades::data_manager->get<resources::terrain>(TerrainInCorner(t, Corner::TOPRIGHT)));
				count = 0;
			}
		}

		//add the bottoms of the last row
		auto iterator = tiles.cend() - tilesPerRow;
		for (iterator; iterator != tiles.cend(); ++iterator)
			out.push_back(hades::data_manager->get<resources::terrain>(TerrainInCorner(*iterator, Corner::BOTTOMLEFT)));

		out.push_back(hades::data_manager->get<resources::terrain>(TerrainInCorner(*tiles.crbegin(), Corner::BOTTOMLEFT)));

		return std::make_tuple(out, ++tilesPerRow);
	}

	TileMap::TileMap(const MapData &map)
	{
		create(map);
	}

	std::array<sf::Vertex, VertexPerTile> CreateTile(hades::types::int32 left, hades::types::int32 top, hades::types::int32 texLeft, hades::types::int32 texTop, hades::types::uint16 tile_size)
	{
		std::array<sf::Vertex, VertexPerTile> output;
		// top-left
		output[0] = { { static_cast<float>(left), static_cast<float>(top) }, { static_cast<float>(texLeft), static_cast<float>(texTop) } };
		//top-right
		output[1] = { { static_cast<float>(left + tile_size), static_cast<float>(top) }, { static_cast<float>(texLeft + tile_size), static_cast<float>(texTop) } };
		//bottom-right
		output[2] = { { static_cast<float>(left + tile_size), static_cast<float>(top + tile_size) },
		{ static_cast<float>(texLeft + tile_size), static_cast<float>(texTop + tile_size) } };
		//bottom-left
		output[3] = { { static_cast<float>(left), static_cast<float>(top + tile_size) }, { static_cast<float>(texLeft), static_cast<float>(texTop + tile_size) } };
		output[4] = output[0];
		output[5] = output[2];

		return output;
	}

	void TileMap::create(const MapData &map_data)
	{
		auto &tiles = std::get<TileArray>(map_data);
		auto width = std::get<tile_count_t>(map_data);

		//generate a drawable array
		Chunks.clear();

		auto settings = tiles::GetTileSettings();
		auto tile_size = settings.tile_size;

		tile_count_t count = 0;

		struct Tile {
			hades::types::int32 x, y;
			tile t;
		};

		std::vector<std::pair<hades::resources::texture*, Tile>> map;
		std::vector<hades::resources::texture*> tex_cache;
		for (auto &t : tiles)
		{
			if (t.terrain != hades::data::UniqueId::Zero)
			{
				hades::resources::texture* texture = nullptr;

				for (auto &c : tex_cache)
				{
					if (c->id == t.texture)
						texture = c;
				}

				if (!texture)
				{
					texture = hades::data_manager->getTexture(t.texture);
					tex_cache.push_back(texture);
				}

				assert(texture);

				hades::types::int32 x, y;

				std::tie(x, y) = tiles::GetGridPosition(count, width, tile_size);

				Tile ntile;
				ntile.x = x;
				ntile.y = y;
				ntile.t = t;

				map.push_back(std::make_pair(texture, ntile));
			}
			count++;
		}

		//sort the tiles by texture;
		std::sort(map.begin(), map.end(), [](const auto &lhs, const auto &rhs) {
			return lhs.first < rhs.first;
		});

		auto current_tex = map.front().first;
		VertexArray array;

		for (auto &t : map)
		{
			//change array
			if (t.first != current_tex)
			{
				Chunks.push_back(std::make_pair(current_tex, array));
				current_tex = t.first;
				array.clear();
			}

			//set vertex position and tex coordinates
			auto vertex_tile = CreateTile(t.second.x, t.second.y, t.second.t.left, t.second.t.top, tile_size);
			for(const auto& v : vertex_tile)
				array.push_back(v);
		}

		Chunks.push_back(std::make_pair(current_tex, array));
	}

	void TileMap::draw(sf::RenderTarget& target, sf::RenderStates states) const
	{
		for (auto &s : Chunks)
		{
			states.texture = &s.first->value;
			states.transform * getTransform();
			target.draw(s.second.data(), s.second.size(), sf::Triangles, states);
		}
	}

	sf::FloatRect TileMap::getLocalBounds() const
	{
		if (Chunks.empty() || Chunks[0].second.empty())
			return sf::FloatRect();

		const auto &first = Chunks[0].second[0].position;

		float top = first.y,
			left = first.x, 
			right = first.x, 
			bottom = first.y;

		for (const auto &varray : Chunks)
		{
			for (const auto &v : varray.second)
			{
				if (v.position.x > right)
					right = v.position.x;
				else if (v.position.x < left)
					left = v.position.x;

				if (v.position.y > bottom)
					bottom = v.position.y;
				else if (v.position.y < top)
					top = v.position.y;
			}
		}

		return sf::FloatRect(left, top, right - left, bottom - top);
	}

	EditMap::EditMap(const MapData &map)
	{
		create(map);
	}

	void EditMap::create(const MapData &map_data)
	{
		//create the vertex map
		auto vert = ConvertToVertex(map_data);
		_vertex = std::get<TerrainVertex>(vert);
		_vertex_width = std::get<tile_count_t>(vert);

		//store the tiles and width
		_tiles = std::get<TileArray>(map_data);
		_width = std::get<tile_count_t>(map_data);

		auto settings = tiles::GetTileSettings();
		_tile_size = settings.tile_size;

		TileMap::create(map_data);
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

	void EditMap::replace(const tile& t, const sf::Vector2u &position, hades::types::uint8 amount, bool updateVertex)
	{	
		if (t.terrain == hades::data::UniqueId::Zero)
			return;

		auto positions = AllPositions(position, amount);

		for (const auto &p : positions)
		{
			auto position = sf::Vector2u(p);
			//tried to place a tile outside the map
			if (!IsWithin(_tiles, position, _width))
				continue;

			//find the array to place it in
			vArray *targetArray = nullptr;

			for (auto &a : Chunks)
			{
				if (a.first->id == t.texture)
				{
					targetArray = &a;
					break;
				}
			}

			//create a new vertex array if needed
			if (!targetArray && hades::data_manager->exists(t.texture))
			{
				auto texture = hades::data_manager->getTexture(t.texture);
				Chunks.push_back({ texture, VertexArray() });
				targetArray = &Chunks.back();
			}
			else if (!targetArray)
			{
				//the texture doesn't exist,
				//this should be impossible at this point,
				//as it should be checked when adding tiles/terrain
				//to the tile selector
				assert(false);
			}

			//find the location of the current tile
			vArray *currentArray = nullptr;

			const auto pixelPos = position * _tile_size;

			for (auto &a : Chunks)
			{
				assert(a.second.size() % VertexPerTile == 0);
				for (std::size_t i = 0; i != a.second.size(); i += VertexPerTile)
				{
					if (static_cast<sf::Vector2u>(a.second[i].position) == pixelPos)
					{
						currentArray = &a;
						break;
					}
				}
			}

			assert(currentArray);

			//if the arrays are the same, then replace the quad
			//otherwise remove the quad from the old array and insert into the new one
			if (currentArray == targetArray)
				_replaceTile(targetArray->second, position, t);
			else
			{
				_removeTile(currentArray->second, position);
				_addTile(targetArray->second, position, t);
			}

			//write the tile to the tile vector
			Write(_tiles, position, _width, t);
			
			if (updateVertex)
			{
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
					const auto *terrain = hades::data_manager->get<resources::terrain>(t_id);
					//write to vertex
					Write(_vertex, verts[i], _vertex_width, terrain);
				}
			}
		}
	}

	void EditMap::replace(const resources::terrain& terrain, const sf::Vector2u &position, hades::types::uint8 amount)
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

	MapData EditMap::getMap() const
	{
		return { _tiles, _width };
	}

	void EditMap::_cleanTransitions(const sf::Vector2u &position, sf::Vector2u size)
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
	
	void EditMap::_removeTile(VertexArray &a, const sf::Vector2u &position)
	{
		auto pixelPos = position * _tile_size;
		VertexArray::const_iterator target = a.cend();

		for (auto iter = a.cbegin(); iter != a.cend(); iter += VertexPerTile)
		{
			if (pixelPos == static_cast<sf::Vector2u>(iter->position))
			{
				target = iter;
				break;
			}
		}

		a.erase(target, target + VertexPerTile);
	}

	void EditMap::_replaceTile(VertexArray &a, const sf::Vector2u &position, const tile& t)
	{
		auto &vertArray = a;
		std::size_t firstVert = vertArray.size();
		auto vertPosition = position * _tile_size;
		for (std::size_t i = 0; i < firstVert; i += VertexPerTile)
		{
			if (static_cast<sf::Vector2u>(vertArray[i].position) == vertPosition)
			{
				firstVert = i;
				break;
			}
		}

		//vertex position not found
		//this is a big error
		if (firstVert == vertArray.size())
		{
			//TODO: log, throw logic_error
			assert(false);
			return;
		}

		auto newQuad = CreateTile(vertPosition.x, vertPosition.y, t.left, t.top, _tile_size);
		for (auto i = firstVert; i < firstVert + VertexPerTile; ++i)
			vertArray[i] = newQuad[i - firstVert];
	}

	void EditMap::_addTile(VertexArray &a, const sf::Vector2u &position, const tile& t)
	{
		auto pixelPos = position * _tile_size;
		auto newQuad = CreateTile(pixelPos.x, pixelPos.y, t.left, t.top, _tile_size);

		for (const auto &q : newQuad)
			a.push_back(q);
	}

	const resources::terrain_settings &GetTerrainSettings()
	{
		auto settings_id = hades::data_manager->getUid(resources::terrain_settings_name);
		auto data_manager = hades::data_manager;
		if (!data_manager->exists(settings_id))
		{
			auto message = "terrain-settings undefined.";
			LOGERROR(message)
				throw exception(message);
		}

		try
		{
			return *hades::data_manager->get<resources::terrain_settings>(settings_id);
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
		tiles::traits_list out;
	}

	terrain_info GetTerrainInfo(const tiles::tile &tile)
	{
		auto it = TerrainLookup.find(tile);
		if (it == std::end(TerrainLookup))
			return terrain_info();

		return *it;
	}
}
