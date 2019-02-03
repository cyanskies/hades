#include "hades/tile_map.hpp"

#include <array>

#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Graphics/Vertex.hpp"

#include "hades/animation.hpp"
#include "hades/data.hpp"
#include "hades/logging.hpp"
#include "hades/texture.hpp"
#include "hades/tiles.hpp"
#include "hades/utility.hpp"

namespace hades
{
	void register_tile_map_resources(data::data_manager &d)
	{
		//register dependent resources
		register_texture_resource(d);
		register_tiles_resources(d, [](data::data_manager &d, unique_id id, unique_id mod) {
			return d.find_or_create<resources::texture>(id, mod);
		});

		//add texture to error tile
		const auto settings = resources::get_tile_settings();
		const auto &error_tile = resources::get_error_tile();
		auto error_tex = d.find_or_create<resources::texture>(error_tile.texture->id, unique_id::zero);
		error_tex->height = error_tex->width = settings->tile_size;
		error_tex->repeat = true;
		sf::Image img{};
		img.create(1u, 1u, nullptr);
		img.setPixel(0u, 0u, sf::Color::Magenta);
		error_tex->value.loadFromImage(img);
		error_tex->value.setRepeated(true);
		error_tex->value.setSmooth(false);
		error_tex->loaded = true;
	}

	//=========================================//
	//					immutable_tile_map				   //
	//=========================================//

	immutable_tile_map::immutable_tile_map(const tile_map &map)
	{
		create(map);
	}

	std::tuple<tile_count_t, tile_count_t> to_2d_position(tile_count_t width, std::size_t index)
	{
		return {
			index % width,
			index / width			
		};
	}

	void immutable_tile_map::create(const tile_map &map_data)
	{
		const auto &tiles = map_data.tiles;
		const auto width = map_data.width;

		//generate a drawable array
		texture_layers.clear();

		const auto settings = resources::get_tile_settings();
		const auto tile_size = settings->tile_size;

		local_bounds = rect_float{
			0.f, 0.f,
			static_cast<float>(width * tile_size),
			static_cast<float>((tiles.size() / width) * tile_size)
		};

		struct map_tile {
			const resources::texture *texture = nullptr;
			tile_count_t x{}, y{};
			const resources::tile *tile = nullptr;
		};

		//collect info about each cell in the map
		std::vector<map_tile> map;
		std::vector<const hades::resources::texture*> tex_cache;
		for (auto i = std::size_t{}; i < std::size(tiles); ++i)
		{
			const auto &t = get_tile(map_data, tiles[i]);

			//skip 'empty' tiles, no need to render a transparent texture
			if (!t.texture)
				continue;

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
			const auto [x, y] = to_2d_position(i, width);

			const map_tile ntile{ texture, x * tile_size, y *tile_size, &t };
			map.emplace_back(ntile);
		}

		//sort the tiles by texture;
		std::sort(map.begin(), map.end(), [](const auto &lhs, const auto &rhs) {
			constexpr auto less = std::less<const resources::texture*>{};
			return less(lhs.first, rhs.first);
		});

		//whole map is tranparent
		//exit early
		if (map.empty())
			return;

		auto current_tex = map.front().texture;
		std::vector<sf::Vertex> v_array;
		v_array.reserve(map.size() * std::tuple_size_v<poly_quad>);

		auto finalise_layer = [](std::vector<texture_layer> &layers, const resources::texture *t, const std::vector<sf::Vertex> &v)
		{
			auto &l = layers.emplace_back(
				texture_layer{
					t,
					vertex_buffer{sf::PrimitiveType::Triangles,sf::VertexBuffer::Usage::Static}
				}
			);

			l.vertex.set_verts(v);
		};

		for (const auto &t : map)
		{
			//change array once we've added every tile with this texture
			if (t.texture != current_tex)
			{
				finalise_layer(texture_layers, current_tex, v_array);

				current_tex = t.texture;
				v_array.clear();
			}

			//float_cast
			const auto quad_rect = rect_float{
				float_cast(t.x),
				float_cast(t.y),
				float_cast(t.x + tile_size),
				float_cast(t.y + tile_size)
			};

			const auto text_rect = rect_float{
				float_cast(t.tile->left),
				float_cast(t.tile->top),
				float_cast(t.tile->left + tile_size),
				float_cast(t.tile->top + tile_size)
			};

			//set vertex position and tex coordinates
			const auto vertex_tile = make_quad_animation(quad_rect, text_rect);

			for (const auto& v : vertex_tile)
				v_array.push_back(v);
		}

		finalise_layer(texture_layers, current_tex, v_array);
	}

	void immutable_tile_map::draw(sf::RenderTarget& target, sf::RenderStates states) const
	{
		for (const auto &s : texture_layers)
		{
			states.texture = &s.texture->value;
			target.draw(s.vertex, states);
		}
	}

	rect_float immutable_tile_map::get_local_bounds() const
	{
		return local_bounds;
	}

	//=========================================//
	//				mutable_tile_map			   //
	//=========================================//

	mutable_tile_map::mutable_tile_map(const MapData &map)
	{
		create(map);
	}

	void mutable_tile_map::create(const MapData &map_data)
	{
		//store the tiles and width
		_tiles = std::get<TileArray>(map_data);
		_width = std::get<tile_count_t>(map_data);

		const auto tile_settings = GetTileSettings();
		_tile_size = tile_settings->tile_size;

		immutable_tile_map::create(map_data);
	}

	sf::Vector2u InflatePosition(tile_count_t i, tile_count_t width)
	{
		const auto x = i % width;
		const auto y = i / width;

		return { x, y };
	}

	void mutable_tile_map::update(const MapData &map_data)
	{
		const auto &[tiles, width] = map_data;
		if (tiles.size() != _tiles.size()
			|| width != _width)
			throw tile_map_exception("immutable_tile_map::update must be called with a map of the same size and width");
		
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

	void mutable_tile_map::replace(const tile& t, const sf::Vector2i &position,draw_size_t amount)
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

	MapData mutable_tile_map::getMap() const
	{
		return { _tiles, _width };
	}

	void mutable_tile_map::setColour(sf::Color c)
	{
		_colour = c;
		for (auto &a : texture_layers)
		{
			for (auto &v : a.second)
				v.color = c;
		}
	}

	void mutable_tile_map::_updateTile(const sf::Vector2u &position, const tile &t)
	{
		//find the array to place it in
		VertexArray *targetArray = nullptr;

		for (auto &a : texture_layers)
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
			texture_layers.push_back({ t.texture, VertexArray() });
			targetArray = &texture_layers.back();
		}

		//ensure a new array has been selected to place the tiles into
		assert(targetArray);

		//find the location of the current tile
		VertexArray *currentArray = nullptr;

		const auto pixelPos = position * _tile_size;

		for (auto &a : texture_layers)
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

	void mutable_tile_map::_removeTile(VertexArray &a, const sf::Vector2u &position)
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

	void mutable_tile_map::_replaceTile(VertexArray &a, const sf::Vector2u &position, const tile& t)
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
			LOGERROR("immutable_tile_map; unable to find vertex for modification");
			throw std::logic_error("failed to find needed vertex");
		}

		auto newQuad = CreateTile(vertPosition.x, vertPosition.y, t.left, t.top, _tile_size);

		for (auto &v : newQuad)
			v.color = _colour;

		for (auto i = firstVert; i < firstVert + VertexPerTile; ++i)
			vertArray[i] = newQuad[i - firstVert];
	}

	void mutable_tile_map::_addTile(VertexArray &a, const sf::Vector2u &position, const tile& t)
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
