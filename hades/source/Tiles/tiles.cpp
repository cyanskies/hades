#include "Tiles/tiles.hpp"

#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Graphics/Vertex.hpp"

#include "Hades/Data.hpp"
#include "Hades/DataManager.hpp"
#include "Hades/data_manager.hpp"
#include "Hades/Utility.hpp"

namespace tiles
{
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
			auto start_id = std::get<tile_count_t>(t);
			auto tileset_id = std::get<hades::data::UniqueId>(t);
			auto tileset = hades::data::Get<resources::tileset>(tileset_id);
			assert(tileset);
			tilesets.push_back({ tileset, start_id });
		}

		auto &source_tiles = std::get<std::vector<tile_count_t>>(map);
		auto &tiles = std::get<TileArray>(out);
		tiles.reserve(source_tiles.size());
		//for each tile, get it's actual tile value
		for (auto &t : source_tiles)
		{
			Tileset tset = { nullptr, 0 };
			for (const auto &s : tilesets)
			{
				if (t >= std::get<tile_count_t>(s))
					tset = s;
				else
					break;
			}

			auto tileset = std::get<const resources::tileset*>(tset);
			auto count = std::get<tile_count_t>(tset);
			assert(tileset);
			count = t - count;
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
		auto &tiles = std::get<TileArray>(map_data);
		auto width = std::get<tile_count_t>(map_data);

		//generate a drawable array
		Chunks.clear();

		using namespace resources;
		auto settings_id = hades::data::GetUid(tile_settings_name);

		if (!hades::data::Exists(settings_id))
			throw tile_map_exception("Missing tile settings resource");

		auto settings = hades::data::Get<tile_settings>(settings_id);
		auto tile_size = settings->tile_size;

		tile_count_t count = 0;

		struct Tile {
			hades::types::int32 x, y;
			tile t;
		};

		std::vector<std::pair<const hades::resources::texture*, Tile>> map;
		std::vector<const hades::resources::texture*> tex_cache;
		for (auto &t : tiles)
		{
			const hades::resources::texture* texture = nullptr;

			for (auto &c : tex_cache)
			{
				if (c->id == t.texture)
					texture = c;
			}

			if (!texture)
			{
				texture = hades::data::Get<hades::resources::texture>(t.texture);
				tex_cache.push_back(texture);
			}

			assert(texture);

			hades::types::int32 x, y;

			std::tie(x, y) = GetGridPosition(count, width, tile_size);

			Tile ntile;
			ntile.x = x;
			ntile.y = y;
			ntile.t = t;

			map.push_back(std::make_pair(texture, ntile));

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
			for (const auto& v : vertex_tile)
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

		auto tile_settings = GetTileSettings();
		_tile_size = tile_settings.tile_size;

		TileMap::create(map_data);
	}

	std::vector<sf::Vector2i> AllPositions(const sf::Vector2u &position, hades::types::uint8 amount)
	{
		if (amount == 0)
			return { sf::Vector2i(position) };

		auto end = sf::Vector2i(position) +
			static_cast<sf::Vector2i>(sf::Vector2f{ std::floorf(amount / 2.f), std::floorf(amount / 2.f) });
		auto start_position = sf::Vector2i(position) -
			static_cast<sf::Vector2i>(sf::Vector2f{ std::ceilf(amount / 2.f), std::ceilf(amount / 2.f) });

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

	void MutableTileMap::replace(const tile& t, const sf::Vector2u &position, hades::types::uint8 amount)
	{
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
			if (!targetArray && hades::data::Exists(t.texture))
			{
				auto texture = hades::data::Get<hades::resources::texture>(t.texture);
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
		}
	}

	MapData MutableTileMap::getMap() const
	{
		return { _tiles, _width };
	}

	void MutableTileMap::_removeTile(VertexArray &a, const sf::Vector2u &position)
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

	void MutableTileMap::_replaceTile(VertexArray &a, const sf::Vector2u &position, const tile& t)
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
			LOGERROR("TileMap; unable to find vertex for modification");
			throw std::logic_error("failed to find needed vertex");
		}

		auto newQuad = CreateTile(vertPosition.x, vertPosition.y, t.left, t.top, _tile_size);
		for (auto i = firstVert; i < firstVert + VertexPerTile; ++i)
			vertArray[i] = newQuad[i - firstVert];
	}

	void MutableTileMap::_addTile(VertexArray &a, const sf::Vector2u &position, const tile& t)
	{
		auto pixelPos = position * _tile_size;
		auto newQuad = CreateTile(pixelPos.x, pixelPos.y, t.left, t.top, _tile_size);

		for (const auto &q : newQuad)
			a.push_back(q);
	}

	const resources::tile_settings &GetTileSettings()
	{
		auto settings_id = hades::data::GetUid(resources::tile_settings_name);
		if (!hades::data::Exists(settings_id))
		{
			auto message = "tile-settings undefined. GetTileSettings()";
			LOGERROR(message)
			throw tile_map_exception(message);
		}

		try
		{
			return *hades::data::Get<resources::tile_settings>(settings_id);
		}
		catch (hades::data::resource_wrong_type&)
		{
			auto message = "The UID for the tile settings has been reused for another resource type";
			LOGERROR(message);
			throw tile_map_exception(message);
		}
	}

	const TileArray &GetErrorTileset()
	{
		auto settings = GetTileSettings();
		auto tset = hades::data::Get<resources::tileset>(settings.error_tileset);

		if (!tset)
		{
			LOGWARNING("No error tileset");
			tset = hades::data::Get<resources::tileset>(resources::Tilesets.front());
		}

		return tset->tiles;
	}

	tile GetErrorTile()
	{
		auto tset = GetErrorTileset();
		if (tset.empty())
			return tile();

		auto i = hades::random(0u, tset.size() - 1);
		return tset[i];
	}
}