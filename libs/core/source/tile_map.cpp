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

	vector_t<tile_count_t> to_2d_position(tile_count_t width, std::size_t index)
	{
		return {
			index % width,
			index / width			
		};
	}

	void immutable_tile_map::create(const tile_map &map_data)
	{
		create(map_data, sf::VertexBuffer::Usage::Static);
	}

	void immutable_tile_map::draw(sf::RenderTarget& target, sf::RenderStates states) const
	{
		for (const auto &s : texture_layers)
		{
			states.texture = &s.texture->value;
			target.draw(s.buffer, states);
		}
	}

	rect_float immutable_tile_map::get_local_bounds() const
	{
		return local_bounds;
	}

	void immutable_tile_map::create(const tile_map &map_data, sf::VertexBuffer::Usage usage)
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
			const auto p = to_2d_position(i, width);

			const map_tile ntile{ texture, p.x * tile_size, p.y *tile_size, &t };
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

		auto finalise_layer = [](std::vector<texture_layer> &layers,
			const resources::texture *t, const std::vector<sf::Vertex> &v,
			sf::VertexBuffer::Usage u)
		{
			auto &l = layers.emplace_back(
				texture_layer{
					t,
					v,
					vertex_buffer{sf::PrimitiveType::Triangles, u}
				}
			);

			l.buffer.set_verts(l.vertex);
		};

		for (const auto &t : map)
		{
			//change array once we've added every tile with this texture
			if (t.texture != current_tex)
			{
				finalise_layer(texture_layers, current_tex, v_array, usage);

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

		finalise_layer(texture_layers, current_tex, v_array, usage);
	}

	//=========================================//
	//				mutable_tile_map			   //
	//=========================================//

	constexpr auto vertex_usage = sf::VertexBuffer::Usage::Dynamic;

	mutable_tile_map::mutable_tile_map(const tile_map &map)
	{
		create(map);
	}

	void mutable_tile_map::create(const tile_map &map_data)
	{
		//store the tiles and width
		_tiles = map_data;
		immutable_tile_map::create(map_data, vertex_usage);
	}

	void mutable_tile_map::update(const tile_map &map)
	{
		if (map.tiles.size() != _tiles.tiles.size()
			|| map.width != _tiles.width)
			throw tile_map_error("immutable_tile_map::update must be called with a map of the same size and width");
		
		for (auto i = std::size_t{}; i < map.tiles.size(); ++i)
		{
			if (map.tiles[i] != _tiles.tiles[i])
			{
				const auto &t = get_tile(map, i);
				_update_tile(to_2d_position(map.width, i), t);
			}
		}

		_tiles = map;
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

	void mutable_tile_map::place_tile(tile_position p, const resources::tile &t)
	{
		hades::place_tile(_tiles, p, t);
		_update_tile(p, t);
	}

	void mutable_tile_map::place_tile(const std::vector<tile_position> &positions, const resources::tile &t)
	{
		hades::place_tile(_tiles, positions, t);
		
		for(auto p : positions)
			_update_tile(p, t);
	}

	tile_map mutable_tile_map::get_map() const
	{
		return _tiles;
	}

	void mutable_tile_map::_update_tile(tile_position p, const resources::tile &t)
	{
		//find the array to place it in
		texture_layer *layer = nullptr;

		for (auto &a : texture_layers)
		{
			if (a.texture == t.texture)
			{
				layer = &a;
				break;
			}
		}

		//create a new vertex array if needed
		if (!layer)
		{
			texture_layer l{ t.texture };
			l.buffer = vertex_buffer{ sf::PrimitiveType::Triangles, sf::VertexBuffer::Usage::Dynamic };

			layer = &texture_layers.emplace_back(l);
		}

		//ensure a new array has been selected to place the tiles into
		assert(layer);

		//find the location of the current tile
		texture_layer *current_layer = nullptr;

		const auto tile_size = resources::get_tile_settings()->tile_size;

		const auto pixel_pos = p * signed_cast(tile_size);

		constexpr auto vert_per_tile = std::tuple_size_v<poly_quad>;

		for (auto &l : texture_layers)
		{
			for (auto i = std::size_t{}; i != l.vertex.size(); i += vert_per_tile)
			{
				//TODO: is this a safe conversion?
				vector_int p{
					l.vertex[i].position.x,
					l.vertex[i].position.y
				};

				if (p == pixel_pos)
				{
					current_layer = &l;
					break;
				}
			}
		}

		//if the arrays are the same, then replace the quad
		//otherwise remove the quad from the old array and insert into the new one
		if (layer == current_layer)
		{
			_replace_tile(*layer, p, t);
		}
		else if (current_layer)
		{
			_remove_tile(*current_layer, p);
			_add_tile(*layer, p, t);
		}
		else // tile isn't currently in an array, just add the tile
		{
			_add_tile(*layer, p, t);
		}
	}

	void mutable_tile_map::_remove_tile(texture_layer &l, tile_position p)
	{
		const auto tile_size = resources::get_tile_settings()->tile_size;
		const auto pixel_pos = p * signed_cast(tile_size);
		auto target = std::cend(l.vertex);

		constexpr auto verts_per_tile = std::tuple_size_v<poly_quad>;
		for (auto iter = std::cbegin(l.vertex); iter != std::cend(l.vertex); iter += verts_per_tile)
		{
			auto p = vector_int{
				iter->position.x,
				iter->position.y
			};

			if (pixel_pos == p)
			{
				target = iter;
				break;
			}
		}
		auto end = target;
		std::advance(end, verts_per_tile);
		l.vertex.erase(target, end);
		l.buffer.set_verts(l.vertex);
	}

	void mutable_tile_map::_replace_tile(texture_layer &l, tile_position p, const resources::tile &t)
	{
		auto target = std::end(l.vertex);
		const auto tile_size = resources::get_tile_settings()->tile_size;
		const auto pixel_pos = p * signed_cast(tile_size);
		constexpr auto verts_per_tile = std::tuple_size_v<poly_quad>;
		for (auto iter = std::begin(l.vertex); iter != std::end(l.vertex); std::advance(iter, verts_per_tile))
		{
			const auto p = vector_int{
				iter->position.x,
				iter->position.y
			};

			if (p == pixel_pos)
			{
				target = iter;
				break;
			}
		}

		//vertex position not found
		//this is a big error
		if (target == std::end(l.vertex))
		{
			throw tile_map_error("failed to find needed vertex");
		}

		const auto rect = rect_float{
			pixel_pos.x,
			pixel_pos.y,
			pixel_pos.x + tile_size,
			pixel_pos.y + tile_size
		};

		const auto tex_rect = rect_float{
			t.left,
			t.top,
			t.left + tile_size,
			t.top + tile_size
		};

		const auto quad = make_quad_animation(rect, tex_rect);
		std::copy(std::begin(quad), std::end(quad), target);

		l.buffer.set_verts(l.vertex);
	}

	void mutable_tile_map::_add_tile(texture_layer &l, tile_position p, const resources::tile &t)
	{
		const auto tile_size = resources::get_tile_settings()->tile_size;
		const auto pixel_pos = p * signed_cast(tile_size);

		const auto quad_rect = rect_float{
			pixel_pos.x,
			pixel_pos.y,
			pixel_pos.x + tile_size,
			pixel_pos.y + tile_size
		};

		const auto tex_rect = rect_float{
			t.left,
			t.top,
			t.left + tile_size,
			t.top + tile_size
		};

		const auto quad = make_quad_animation(quad_rect, tex_rect);

		std::copy(std::begin(quad), std::end(quad), std::back_inserter(l.vertex));
		l.buffer.set_verts(l.vertex);
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
