#include "hades/tile_map.hpp"

#include <array>

#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Graphics/Texture.hpp"
#include "SFML/Graphics/Vertex.hpp"

#include "hades/animation.hpp"
#include "hades/data.hpp"
#include "hades/logging.hpp"
#include "hades/texture.hpp"
#include "hades/tiles.hpp"
#include "hades/utility.hpp"

namespace hades
{
	namespace tex_funcs = resources::texture_functions;

	void register_tile_map_resources(data::data_manager &d)
	{
		//register dependent resources
		register_texture_resource(d);
		register_tiles_resources(d, [](data::data_manager &d, unique_id id, unique_id mod) {
			return tex_funcs::find_create_texture(d, id, mod);
		});

		//add texture to error tile
		//const auto settings = resources::get_tile_settings();
		//const auto &error_tile = resources::get_error_tile();
		const auto settings = 
			d.find_or_create<resources::tile_settings>(resources::get_tile_settings_id(), unique_id::zero);
		assert(settings);
		assert(settings->error_tileset);
		assert(!settings->error_tileset->tiles.empty());
		const auto &error_tile = settings->error_tileset->tiles[0];
		auto error_tex = tex_funcs::find_create_texture(d, tex_funcs::get_id(error_tile.texture), unique_id::zero);
		sf::Image img{};
		img.create(1u, 1u, sf::Color::Magenta);
		auto& tex = tex_funcs::get_sf_texture(error_tex);
		tex.loadFromImage(img);
		tex_funcs::set_settings(error_tex, { settings->tile_size, settings->tile_size }, 
			false, true, false, true);
		return;
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
			states.texture = &tex_funcs::get_sf_texture(s.texture);
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
			resources::tile tile;
		};

		//collect info about each cell in the map
		std::vector<map_tile> map;
		std::vector<const hades::resources::texture*> tex_cache;
		for (auto i = std::size_t{}; i < std::size(tiles); ++i)
		{
			const auto t = get_tile(map_data, tiles[i]);

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
			const auto p = to_2d_position(width, i);

			const map_tile ntile{ texture, p.x * tile_size, p.y *tile_size, t };
			map.emplace_back(ntile);
		}

		//sort the tiles by texture;
		std::sort(map.begin(), map.end(), [](const auto &lhs, const auto &rhs) {
			constexpr auto less = std::less<const resources::texture*>{};
			return less(lhs.texture, rhs.texture);
		});

		//whole map is transparent
		//exit early
		if (map.empty())
			return;

		auto current_tex = map.front().texture;
		std::vector<sf::Vertex> v_array;
		v_array.reserve(map.size() * std::tuple_size_v<poly_quad>);

		auto finalise_layer = [](std::vector<texture_layer> &layers,
			const resources::texture *t, std::vector<sf::Vertex> v,
			sf::VertexBuffer::Usage u)
		{
			auto &l = layers.emplace_back(
				texture_layer{
					t,
					std::move(v),
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
				finalise_layer(texture_layers, current_tex, std::move(v_array), usage);

				current_tex = t.texture;
				v_array.clear();
			}

			//static_cast<float>
			const auto quad_rect = rect_float{
				static_cast<float>(t.x),
				static_cast<float>(t.y),
				static_cast<float>(tile_size),
				static_cast<float>(tile_size)
			};

			const auto text_rect = rect_float{
				static_cast<float>(t.tile.left),
				static_cast<float>(t.tile.top),
				static_cast<float>(tile_size),
				static_cast<float>(tile_size)
			};

			//set vertex position and tex coordinates
			const auto vertex_tile = make_quad_animation(quad_rect, text_rect);

			v_array.insert(std::end(v_array), std::begin(vertex_tile), std::end(vertex_tile));
		}

		finalise_layer(texture_layers, current_tex, v_array, usage);
	}

	//=========================================//
	//				mutable_tile_map		   //
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
				const auto &t = get_tile(map, map.tiles[i]);
				const auto p = to_2d_index(i, map.width);
				const auto pos = tile_position{
					static_cast<int32>(p.first),
					static_cast<int32>(p.second)
				};

				_update_tile(pos, t);
			}
		}

		_tiles = map;
	}

	void mutable_tile_map::place_tile(const std::vector<tile_position> &positions, const resources::tile &t)
	{
		if (_tiles.width == 0)
			throw tile_map_error{ "tried to place tile on empty tile_map" };

		hades::place_tile(_tiles, positions, t);
		
		const auto level_y = signed_cast(_tiles.tiles.size() / _tiles.width);
		const auto level_x = signed_cast(_tiles.width);
		for (auto p : positions)
		{
			if (p.x < 0 ||
				p.y < 0 ||
				p.x >= level_x ||
				p.y >= level_y)
				continue;

			_update_tile(p, t);
		}
	}

	tile_map mutable_tile_map::get_map() const
	{
		return _tiles;
	}

	void mutable_tile_map::_update_tile(tile_position p, const resources::tile &t)
	{
		//find the array to place it in
		texture_layer *layer = nullptr;

		if (t.texture)
		{
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
		}

		//find the location of the current tile
		texture_layer *current_layer = nullptr;

		const auto tile_size = resources::get_tile_settings()->tile_size;

		const auto pixel_pos = p * integer_cast<int32>(tile_size);

		constexpr auto vert_per_tile = std::tuple_size_v<poly_quad>;

		for (auto &l : texture_layers)
		{
			for (auto i = std::size_t{}; i != l.vertex.size(); i += vert_per_tile)
			{
				//TODO: is this a safe conversion?
				//we must loose fractional component
				vector_int pos{
					static_cast<vector_int::value_type>(l.vertex[i].position.x),
					static_cast<vector_int::value_type>(l.vertex[i].position.y)
				};

				if (pos == pixel_pos)
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
			if(layer) // IF !LAYER, then both layers are null, just skip
				_replace_tile(*layer, pixel_pos, tile_size, t);
		}
		else if (current_layer)
		{
			_remove_tile(*current_layer, pixel_pos);
			if(layer)//only write into a new layer if we have one
				_add_tile(*layer, pixel_pos, tile_size, t);
			else
			{
				//remove layer if its empty
				if (std::empty(current_layer->vertex))
					_remove_layer(current_layer->texture);
			}
		}
		else // tile isn't currently in an array, just add the tile
		{
			_add_tile(*layer, pixel_pos, tile_size, t);
		}
	}

	void mutable_tile_map::_remove_tile(texture_layer &l, tile_position pos)
	{
		auto target = std::cend(l.vertex);

		constexpr auto verts_per_tile = std::tuple_size_v<poly_quad>;
		for (auto iter = std::cbegin(l.vertex); iter != std::cend(l.vertex); iter += verts_per_tile)
		{
			auto p = vector_int{
				static_cast<vector_int::value_type>(iter->position.x),
				static_cast<vector_int::value_type>(iter->position.y)
			};

			if (pos == p)
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

	void mutable_tile_map::_remove_layer(const resources::texture *t)
	{
		assert(t);
		const auto iter = std::find_if(std::begin(texture_layers),
			std::end(texture_layers), [t](auto &layer) {
			return t == layer.texture;
		});

		texture_layers.erase(iter);
	}

	void mutable_tile_map::_replace_tile(texture_layer &l, tile_position pos, resources::tile_size_t tile_size, const resources::tile &t)
	{
		auto target = std::end(l.vertex);
		constexpr auto verts_per_tile = std::tuple_size_v<poly_quad>;
		for (auto iter = std::begin(l.vertex); iter != std::end(l.vertex); std::advance(iter, verts_per_tile))
		{
			const auto p = vector_int{
				static_cast<vector_int::value_type>(iter->position.x),
				static_cast<vector_int::value_type>(iter->position.y)
			};

			if (p == pos)
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
			static_cast<float>(pos.x),
			static_cast<float>(pos.y),
			static_cast<float>(tile_size),
			static_cast<float>(tile_size)
		};

		const auto tex_rect = rect_float{
			static_cast<float>(t.left),
			static_cast<float>(t.top),
			static_cast<float>(tile_size),
			static_cast<float>(tile_size)
		};

		const auto quad = make_quad_animation(rect, tex_rect);
		std::copy(std::begin(quad), std::end(quad), target);

		l.buffer.set_verts(l.vertex);
	}

	void mutable_tile_map::_add_tile(texture_layer &l, tile_position pixel_pos, resources::tile_size_t tile_size, const resources::tile &t)
	{
		const auto quad_rect = rect_float{
			static_cast<float>(pixel_pos.x),
			static_cast<float>(pixel_pos.y),
			static_cast<float>(tile_size),
			static_cast<float>(tile_size)
		};

		const auto tex_rect = rect_float{
			static_cast<float>(t.left),
			static_cast<float>(t.top),
			static_cast<float>(tile_size),
			static_cast<float>(tile_size)
		};

		const auto quad = make_quad_animation(quad_rect, tex_rect);

		l.vertex.insert(std::end(l.vertex), std::begin(quad), std::end(quad));
		l.buffer.set_verts(l.vertex);
	}
}
