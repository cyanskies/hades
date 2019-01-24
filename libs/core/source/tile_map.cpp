#include "hades/tile_map.hpp"

#include <array>

#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Graphics/Vertex.hpp"

#include "hades/data.hpp"
#include "hades/logging.hpp"
#include "hades/texture.hpp"
#include "hades/utility.hpp"

//functions for loading and parsing tile data

namespace hades
{
	using namespace std::string_view_literals;
	constexpr auto tile_settings_name = "tile-settings"sv;

	namespace ids
	{
		unique_id tile_settings = unique_id::zero;
	}

	void register_tiles(data::data_manager &d)
	{
		//tiles depends on texture
		register_texture_resource(d);

		
		//create texture for error_tile
		const unique_id error_tile_texture{};
		auto error_t_tex = d.find_or_create<hades::resources::texture>(error_tile_texture, unique_id::zero);
		/*error_t_tex->loaded = true;
		error_t_tex->value.loadFromMemory(error_tile::data, error_tile::len);*/

		
		//create texture for empty tile
		const auto empty_tile_texture = hades::unique_id{};
		auto empty_t_tex = d.find_or_create<hades::resources::texture>(empty_tile_texture, unique_id::zero);
		sf::Image empty_i;
		empty_i.create(1u, 1u, sf::Color::Transparent);
		empty_t_tex->value.loadFromImage(empty_i);
		empty_t_tex->value.setRepeated(true);
		empty_t_tex->repeat = true;

	}

	std::tuple<tile_count_t, tile_count_t> CalculateTileCount(std::tuple<hades::level_size_t, hades::level_size_t> size, tile_size_t tile_size)
	{
		auto[x, y] = size;
		if (x % tile_size != 0
			|| y % tile_size != 0)
			throw tile_map_exception("Level size was not multiple of tile size");

		return { x / tile_size, y / tile_size };
	}

	constexpr VertexArray::size_type VertexPerTile = 6u;

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
			const auto tileset_id = std::get<hades::unique_id>(t);
			const auto tileset = hades::data::get<resources::tileset>(tileset_id);
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

	hades::types::int32 find_tile_number(const TileArray &tiles, const tile &t)
	{
		for (std::size_t i = 0; i < tiles.size(); ++i)
		{
			if (t == tiles[i])
				return static_cast<hades::types::int32>(i);
		}

		return -1;
	}

	std::vector<std::tuple<const resources::tileset*, tile_count_t>> get_tilesets_for_map(TileArray tiles)
	{
		struct tileset_info {
			const resources::tileset* tileset = nullptr;
			hades::types::int32 max_id = -1;
		};

		std::vector<tileset_info> tilesets;

		for (const auto &tset_id : resources::Tilesets)
		{
			const auto tset = hades::data::get<resources::tileset>(tset_id);
			tilesets.push_back({ tset });
		}

		//remove duplicate tiles, we're only interested in one of each unique tile
		hades::remove_duplicates(tiles);
		
		for (const auto &t : tiles)
		{
			for (auto &tset : tilesets)
			{
				auto tile_number = find_tile_number(tset.tileset->tiles, t);
				if (tile_number != -1)
				{
					tset.max_id = tset.tileset->tiles.size();
					break;
				}
			}
		}

		std::vector<std::tuple<const resources::tileset*, tile_count_t>> out;

		for (const auto &tset : tilesets)
		{
			if (tset.max_id >= 0)
				out.push_back({ tset.tileset, tset.max_id });
		}

		return out;
	}

	RawMap as_rawmap(const MapData &map)
	{
		assert(std::get<0>(map).size() % std::get<1>(map) == 0);
		//store the map width
		const auto width = std::get<tile_count_t>(map);

		const auto &tiles = std::get<TileArray>(map);
		//a list of tilesets, to tilesets starting id
		const auto tilesets_max = get_tilesets_for_map(tiles);

		auto start_id = 0;

		struct tileset_info {
			const resources::tileset* ptr = nullptr;
			hades::types::int32 start_id = -1;
		};

		std::vector<tileset_info> tilesets;

		for (const auto &tset : tilesets_max)
		{
			tilesets.push_back({ std::get<const resources::tileset*>(tset), start_id });
			start_id += std::get<tile_count_t>(tset);
		}

		std::vector<tile_count_t> tile_list;
		tile_list.reserve(tiles.size());

		//for each tile, add its tileset to the tilesets list,
		// and record the highest tile_id used from that tileset.
		for (const auto &t : tiles)
		{
			//the target tileset will be stored here
			const resources::tileset* tset = nullptr;
			tile_count_t tileset_start_id = 0;
			tile_count_t tile_id = 0;

			for (const auto &tileset : tilesets)
			{
				const auto tileset_ptr = tileset.ptr;
				for (TileArray::size_type i = 0; i < tileset_ptr->tiles.size(); ++i)
				{
					if (t == tileset_ptr->tiles[i])
					{
						tset = tileset_ptr;
						tile_id = i;
						tileset_start_id = tileset.start_id;
						break;
					}
				}

				//tileset has already been found
				if (tset)
					break;
			}

			//by now we should have found the tileset and tile id,
			//if not then saving is impossible
			assert(tset);
			//add whatever was found
			tile_list.push_back(tile_id + tileset_start_id);
		}

		std::vector<TileSetInfo> tileset_output;

		for (const auto &s : tilesets)
			tileset_output.push_back({s.ptr->id, s.start_id});

		return { tileset_output, tile_list, width };
	}

	//converts tile positions in the flat map to a 2d position on the screen
	//NOTE: this returns a pixel position with the maps origin in the top left corner(0, 0).
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
			//skip 'empty' tiles, no need to render a transparent texture
			if (t.texture->id == id::empty_tile_texture)
			{
				++count;
				continue;
			}

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

		//whole map is tranparent
		//exit early
		if (map.empty())
			return;

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

	sf::Vector2u InflatePosition(tile_count_t i, tile_count_t width)
	{
		const auto x = i % width;
		const auto y = i / width;

		return { x, y };
	}

	void MutableTileMap::update(const MapData &map_data)
	{
		const auto &[tiles, width] = map_data;
		if (tiles.size() != _tiles.size()
			|| width != _width)
			throw tile_map_exception("TileMap::update must be called with a map of the same size and width");
		
		for (std::size_t i = 0; i < tiles.size(); ++i)
		{
			if(tiles[i] != _tiles[i])
				_updateTile(InflatePosition(i, width), tiles[i]);
		}

		_tiles = tiles;
	}

	std::vector<sf::Vector2i> AllPositions(const sf::Vector2i &position, tiles::draw_size_t amount)
	{
		if (amount == 0)
			return { position };

		const auto end = position +
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
		//ensure the tile has a valid texture obj
		assert(t.texture);

		const auto positions = AllPositions(position, amount);

		for (const auto &p : positions)
		{
			const auto position = sf::Vector2u(p);
			//tried to place a tile outside the map
			if (!IsWithin(_tiles, position, _width))
				continue;

			_updateTile(position, t);

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

	void MutableTileMap::_updateTile(const sf::Vector2u &position, const tile &t)
	{
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
		if (!targetArray)
		{
			Chunks.push_back({ t.texture, VertexArray() });
			targetArray = &Chunks.back();
		}

		//ensure a new array has been selected to place the tiles into
		assert(targetArray);

		//find the location of the current tile
		vArray *currentArray = nullptr;

		const auto pixelPos = position * _tile_size;

		for (auto &a : Chunks)
		{
			//check the that vertex array still has an expected number of elements
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

		//if the arrays are the same, then replace the quad
		//otherwise remove the quad from the old array and insert into the new one
		if (currentArray == targetArray)
		{
			_replaceTile(targetArray->second, position, t);
		}
		else if (currentArray)
		{
			_removeTile(currentArray->second, position);
			_addTile(targetArray->second, position, t);
		}
		else // tile isn't currently in an array, just add the tile
		{
			_addTile(targetArray->second, position, t);
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
		if (id::tile_settings == hades::unique_id::zero)
		{
			const auto message = "tile-settings undefined. GetTileSettings()";
			LOGERROR(message)
			throw tile_map_exception(message);
		}

		try
		{
			return hades::data::get<resources::tile_settings>(id::tile_settings);
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
			const auto backup_tileset = hades::data::get<resources::tileset>(resources::Tilesets.front());
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

	tile GetEmptyTile()
	{
		static const auto tile_settings = GetTileSettings();
		assert(!tile_settings->empty_tileset->tiles.empty());
		static const auto t = tile_settings->empty_tileset->tiles.front();

		return t;
	}

	constexpr auto tiles = "tiles";
	constexpr auto tilesets = "tilesets";
	constexpr auto map = "map";

	void ReadTileLayerFromYaml(const YAML::Node &t, tile_layer &target)
	{
		// tilesets:
		//     - [name, gid]
		// map: [1,2,3,4...]
		// width: 1

		//tilesets
		const auto tilesets_node = t[tilesets];
		if (tilesets_node.IsDefined() && tilesets_node.IsSequence())
		{
			for (const auto tset : tilesets_node)
			{
				if (!tset.IsSequence() || tset.size() != 2)
				{
					//TODO:
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

		ReadTileLayerFromYaml(t, target.tiles);
	}

	YAML::Emitter &WriteTileLayerToYaml(const tile_layer &l, YAML::Emitter &e)
	{
		e << YAML::BeginMap;
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

	YAML::Emitter &WriteTilesToYaml(const level &l, YAML::Emitter &e)
	{
		//write tiles
		e << YAML::Key << tiles;
		e << YAML::Value;
		
		WriteTileLayerToYaml(l.tiles, e);

		return e;
	}
}
