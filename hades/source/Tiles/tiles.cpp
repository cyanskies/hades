#include "Tiles/tiles.hpp"

#include <array>

#include "yaml-cpp/yaml.h"

#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Graphics/Vertex.hpp"

#include "Hades/Data.hpp"
#include "Hades/Utility.hpp"

#include "Objects/Objects.hpp"

namespace tiles
{
	void EnableTiles(hades::data::data_system *d)
	{
		assert(d);
		objects::EnableObjects(d);
		RegisterTileResources(d);
	}

	namespace
	{
		static const VertexArray::size_type VertexPerTile = 6;
	}

	MapData as_mapdata(const RawMap &map)
	{
		MapData out;
		std::get<tile_count_t>(out) = std::get<tile_count_t>(map);

		using Tileset = std::tuple<const resources::tileset*, tile_count_t>;
		std::vector<Tileset> tilesets;

		//get all the tilesets
		for (const auto &t : std::get<std::vector<TileSetInfo>>(map))
		{
			const auto start_id = std::get<tile_count_t>(t);
			const auto tileset_id = std::get<hades::data::UniqueId>(t);
			const auto tileset = hades::data::Get<resources::tileset>(tileset_id);
			assert(tileset);
			tilesets.push_back({ tileset, start_id });
		}

		const auto &source_tiles = std::get<std::vector<tile_count_t>>(map);
		auto &tiles = std::get<TileArray>(out);
		tiles.reserve(source_tiles.size());
		//for each tile, get it's actual tile value
		for (const auto &t : source_tiles)
		{
			Tileset tset = { nullptr, 0 };
			for (const auto &s : tilesets)
			{
				if (t >= std::get<tile_count_t>(s))
					tset = s;
				else
					break;
			}

			const auto tileset = std::get<const resources::tileset*>(tset);
			const auto count = t - std::get<tile_count_t>(tset);
			assert(tileset);
			assert(tileset->tiles.size() > count);
			tiles.push_back(tileset->tiles[count]);
		}

		assert(source_tiles.size() == std::get<TileArray>(out).size());

		return out;
	}

	RawMap as_rawmap(const MapData &map)
	{
		RawMap output;

		assert(std::get<0>(map).size() % std::get<1>(map) == 0);
		//store the map width
		std::get<tile_count_t>(output) = std::get<tile_count_t>(map);

		//a list of tilesets, to tilesets starting id
		std::vector<std::pair<const resources::tileset*, tile_count_t>> tilesets;

		//for each tile, add its tileset to the tilesets list,
		// and record the highest tile_id used from that tileset.
		const auto &tiles = std::get<TileArray>(map);
		auto &tile_list = std::get<std::vector<tile_count_t>>(output);
		tile_list.reserve(tiles.size());
		for (const auto &t : tiles)
		{
			//the target tileset will be stored here
			const resources::tileset* tset = nullptr;
			tile_count_t tileset_start_id = 0;
			tile_count_t tile_id = 0;
			//check tilesets cache for id
			for (const auto &s : tilesets)
			{
				const auto &set_tiles = s.first->tiles;
				for (std::vector<tile>::size_type i = 0; i < set_tiles.size(); ++i)
				{
					if (set_tiles[i] == t)
					{
						tset = s.first;
						tileset_start_id = s.second;
						tile_id = i;
						break;
					}
				}

				//if we found it then break
				if (tset)
					break;
			}

			//if we didn't have the tileset already, check the master tileset list.
			if (!tset)
			{
				//check Tilesets for the containing tileset
				for (const auto &s_id : resources::Tilesets)
				{
					const auto s = hades::data::Get<resources::tileset>(s_id);
					const auto &set_tiles = s->tiles;
					for (std::vector<tile>::size_type i = 0; i < set_tiles.size(); ++i)
					{
						if (set_tiles[i] == t)
						{
							if (tilesets.empty())
								tileset_start_id = 0;
							else
							{
								const auto &back = tilesets.back();
								tileset_start_id = back.second + set_tiles.size();
							}

							tset = s;
							tilesets.push_back({ s, tileset_start_id });
							tile_id = i;
							break;
						}
					}

					if (tset)
						break;
				}
			}

			//by now we should have found the tileset and tile id,
			//if not then saving is impossible
			assert(tset);
			//add whatever was found
			tile_list.push_back(tile_id + tileset_start_id);
		}

		auto &tilesets_out = std::get<std::vector<TileSetInfo>>(output);

		for (const auto &s : tilesets)
			tilesets_out.push_back(std::make_tuple(s.first->id, s.second));

		return output;
	}

	std::tuple<hades::types::int32, hades::types::int32> GetGridPosition(hades::types::uint32 tile_number,
		hades::types::uint32 tiles_per_row, tile_size_t tile_size)
	{
		return std::make_tuple((tile_number % tiles_per_row) * tile_size,
			static_cast<int>(std::floor(tile_number / static_cast<float>(tiles_per_row)) * tile_size));
	}

	//=========================================//
	//					TileMap				   //
	//=========================================//

	TileMap::TileMap(const MapData &map)
	{
		create(map);
	}

	std::array<sf::Vertex, VertexPerTile> CreateTile(hades::types::int32 left, hades::types::int32 top, hades::types::int32 texLeft, hades::types::int32 texTop, hades::types::uint16 tile_size)
	{
		std::array<sf::Vertex, VertexPerTile> output;
		// top-left
		output[0] = { { static_cast<float>(left), static_cast<float>(top) },{ static_cast<float>(texLeft), static_cast<float>(texTop) } };
		//top-right
		output[1] = { { static_cast<float>(left + tile_size), static_cast<float>(top) },{ static_cast<float>(texLeft + tile_size), static_cast<float>(texTop) } };
		//bottom-right
		output[2] = { { static_cast<float>(left + tile_size), static_cast<float>(top + tile_size) },
		{ static_cast<float>(texLeft + tile_size), static_cast<float>(texTop + tile_size) } };
		//bottom-left
		output[3] = { { static_cast<float>(left), static_cast<float>(top + tile_size) },{ static_cast<float>(texLeft), static_cast<float>(texTop + tile_size) } };
		output[4] = output[0];
		output[5] = output[2];

		return output;
	}

	void TileMap::create(const MapData &map_data)
	{
		const auto &tiles = std::get<TileArray>(map_data);
		const auto width = std::get<tile_count_t>(map_data);

		//generate a drawable array
		Chunks.clear();

		const auto settings = GetTileSettings();
		const auto tile_size = settings->tile_size;

		tile_count_t count = 0;

		struct Tile {
			hades::types::int32 x, y;
			tile t;
		};

		std::vector<std::pair<const hades::resources::texture*, Tile>> map;
		std::vector<const hades::resources::texture*> tex_cache;
		for (const auto &t : tiles)
		{
			const hades::resources::texture* texture = nullptr;

			for (auto &c : tex_cache)
			{
				if (c == t.texture)
					texture = c;
			}

			if (!texture)
			{
				texture = t.texture;
				tex_cache.push_back(texture);
			}

			assert(texture);
			const auto [x, y] = GetGridPosition(count, width, tile_size);

			const Tile ntile{ x, y, t };
			map.push_back(std::make_pair(texture, ntile));

			count++;
		}

		//sort the tiles by texture;
		std::sort(map.begin(), map.end(), [](const auto &lhs, const auto &rhs) {
			return lhs.first < rhs.first;
		});

		auto current_tex = map.front().first;
		VertexArray array;

		for (const auto &t : map)
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
			for (const auto& v : vertex_tile)
				array.push_back(v);
		}

		Chunks.push_back(std::make_pair(current_tex, array));
	}

	void TileMap::draw(sf::RenderTarget& target, sf::RenderStates states) const
	{
		for (const auto &s : Chunks)
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

	//=========================================//
	//				MutableTileMap			   //
	//=========================================//

	MutableTileMap::MutableTileMap(const MapData &map)
	{
		create(map);
	}

	void MutableTileMap::create(const MapData &map_data)
	{
		//store the tiles and width
		_tiles = std::get<TileArray>(map_data);
		_width = std::get<tile_count_t>(map_data);

		const auto tile_settings = GetTileSettings();
		_tile_size = tile_settings->tile_size;

		TileMap::create(map_data);
	}

	std::vector<sf::Vector2i> AllPositions(const sf::Vector2i &position, tiles::draw_size_t amount)
	{
		if (amount == 0)
			return { sf::Vector2i(position) };

		const auto end = sf::Vector2i(position) +
			static_cast<sf::Vector2i>(sf::Vector2f{ std::floor(amount / 2.f), std::floor(amount / 2.f) });
		const auto start_position = sf::Vector2i(position) -
			static_cast<sf::Vector2i>(sf::Vector2f{ std::ceil(amount / 2.f), std::ceil(amount / 2.f) });

		std::vector<sf::Vector2i> positions;

		for (auto r = start_position.x; r < end.x; ++r)
		{
			for (auto c = start_position.y; c < end.y; ++c)
				positions.push_back({ r, c });
		}

		return positions;
	}

	tile_count_t FlatPosition(const sf::Vector2u &position, tile_count_t width)
	{
		return position.y * width + position.x;
	}

	template<class T>
	bool IsWithin(std::vector<T> &dest, const sf::Vector2u &position, tile_count_t width)
	{
		const auto pos = FlatPosition(position, width);
		return pos < dest.size() &&
			position.x < width;
	}

	//returns true if the value in dest was changed
	template<class T>
	bool Write(std::vector<T> &dest, const sf::Vector2u &position, tile_count_t width, const T &value)
	{
		const auto pos = FlatPosition(position, width);
		const auto changed = dest[pos] != value;
		dest[pos] = value;
		return changed;
	}

	void MutableTileMap::replace(const tile& t, const sf::Vector2i &position,draw_size_t amount)
	{
		const auto positions = AllPositions(position, amount);

		for (const auto &p : positions)
		{
			const auto position = sf::Vector2u(p);
			//tried to place a tile outside the map
			if (!IsWithin(_tiles, position, _width))
				continue;

			//find the array to place it in
			vArray *targetArray = nullptr;

			for (auto &a : Chunks)
			{
				if (a.first == t.texture)
				{
					targetArray = &a;
					break;
				}
			}

			//create a new vertex array if needed
			if (!targetArray && t.texture)
			{
				Chunks.push_back({ t.texture, VertexArray() });
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
		}
	}

	MapData MutableTileMap::getMap() const
	{
		return { _tiles, _width };
	}

	void MutableTileMap::setColour(sf::Color c)
	{
		_colour = c;
		for (auto &a : Chunks)
		{
			for (auto &v : a.second)
				v.color = c;
		}
	}

	void MutableTileMap::_removeTile(VertexArray &a, const sf::Vector2u &position)
	{
		const auto pixelPos = position * _tile_size;
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

	void MutableTileMap::_replaceTile(VertexArray &a, const sf::Vector2u &position, const tile& t)
	{
		auto &vertArray = a;
		std::size_t firstVert = vertArray.size();
		const auto vertPosition = position * _tile_size;
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
			LOGERROR("TileMap; unable to find vertex for modification");
			throw std::logic_error("failed to find needed vertex");
		}

		auto newQuad = CreateTile(vertPosition.x, vertPosition.y, t.left, t.top, _tile_size);

		for (auto &v : newQuad)
			v.color = _colour;

		for (auto i = firstVert; i < firstVert + VertexPerTile; ++i)
			vertArray[i] = newQuad[i - firstVert];
	}

	void MutableTileMap::_addTile(VertexArray &a, const sf::Vector2u &position, const tile& t)
	{
		const auto pixelPos = position * _tile_size;
		const auto newQuad = CreateTile(pixelPos.x, pixelPos.y, t.left, t.top, _tile_size);

		for (const auto &q : newQuad)
			a.push_back(q);
	}

	const resources::tile_settings *GetTileSettings()
	{
		const auto settings_id = hades::data::GetUid(resources::tile_settings_name);
		if (!hades::data::Exists(settings_id))
		{
			const auto message = "tile-settings undefined. GetTileSettings()";
			LOGERROR(message)
			throw tile_map_exception(message);
		}

		try
		{
			return hades::data::Get<resources::tile_settings>(settings_id);
		}
		catch (hades::data::resource_wrong_type&)
		{
			const auto message = "The UID for the tile settings has been reused for another resource type";
			LOGERROR(message);
			throw tile_map_exception(message);
		}
	}

	const TileArray &GetErrorTileset()
	{
		const auto settings = GetTileSettings();
		if (!settings->error_tileset)
		{
			LOGWARNING("No error tileset");
			const auto backup_tileset = hades::data::Get<resources::tileset>(resources::Tilesets.front());
			return backup_tileset->tiles;
		}

		return settings->error_tileset->tiles;
	}

	tile GetErrorTile()
	{
		const auto tset = GetErrorTileset();
		if (tset.empty())
			return tile();

		const auto i = hades::random(0u, tset.size() - 1);
		return tset[i];
	}

	constexpr auto tiles = "tiles";
	constexpr auto tilesets = "tilesets";
	constexpr auto map = "map";

	void ReadTilesFromYaml(const YAML::Node &n, level &target)
	{
		//level root
		//
		//tiles:
		//    tilesets:
		//        - [name, gid]
		//    map: [1,2,3,4...]
		//    width: 1

		const auto t = n[tiles];

		if (!t.IsDefined() || !t.IsMap())
			return;

		//tilesets
		const auto tilesets_node = t[tilesets];
		if (tilesets_node.IsDefined() && tilesets_node.IsSequence())
		{
			for (const auto tset : tilesets_node)
			{
				if (!tset.IsSequence() || tset.size() != 2)
				{
					//error, invalid file
				}

				const auto name = tset[0];
				const auto name_str = name.as<hades::types::string>();
				const auto first_id = tset[1];
				const auto id = first_id.as<tile_count_t>();

				target.tilesets.push_back({ name_str, id });
			}
		}

		//map data
		const auto map_node = t[map];
		if (map_node.IsDefined() && map_node.IsSequence())
		{
			for (const auto tile : map_node)
				target.tiles.push_back(tile.as<tile_count_t>());
		}
	}

	YAML::Emitter &WriteTilesToYaml(const level &l, YAML::Emitter &e)
	{
		//write tiles
		e << YAML::Key << tiles;
		e << YAML::Value << YAML::BeginMap;
		
		if (!l.tilesets.empty())
		{
			e << YAML::Key << tilesets;

			e << YAML::Value << YAML::BeginSeq;
			for (const auto &t : l.tilesets)
			{
				e << YAML::Flow << YAML::BeginSeq;
				e << std::get<hades::types::string>(t) << std::get<tile_count_t>(t);
				e << YAML::EndSeq;
			}
			e << YAML::EndSeq;
		}

		if (!l.tiles.empty())
		{
			e << YAML::Key << map;
			e << YAML::Value << YAML::Flow << YAML::BeginSeq;
			for (const auto t : l.tiles)
				e << t;

			e << YAML::EndSeq;
		}

		e << YAML::EndMap;

		return e;
	}
}
