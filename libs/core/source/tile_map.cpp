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

	void immutable_tile_map::create(const tile_map &map_data)
	{
		create(map_data, sf::VertexBuffer::Usage::Static);
	}

	void immutable_tile_map::draw(sf::RenderTarget& target, sf::RenderStates states) const
	{
		for (const auto &s : texture_layers)
		{
			states.texture = &tex_funcs::get_sf_texture(s.texture);
			target.draw(s.quads, states);
		}
	}

	rect_float immutable_tile_map::get_local_bounds() const
	{
		return local_bounds;
	}

	void immutable_tile_map::create(const tile_map &map_data, const sf::VertexBuffer::Usage usage)
	{
		const auto &tiles = map_data.tiles;
		const auto width = map_data.width;

		//generate a drawable array
		texture_layers.clear();

		const auto settings = resources::get_tile_settings();
		const auto tile_size = settings->tile_size;

		local_bounds = rect_float{
			0.f, 0.f,
			float_cast(width * tile_size),
			float_cast((tiles.size() / width) * tile_size)
		};

		struct map_tile {
			const resources::texture *texture = nullptr;
			tile_index_t index;
			resources::tile tile;
		};

		//collect info about each cell in the map
		std::vector<map_tile> map;
		std::vector<const hades::resources::texture*> tex_cache;
		const auto map_end = integer_cast<tile_index_t>(size(tiles));
		for (auto i = tile_index_t{}; i < map_end; ++i)
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
			const map_tile ntile{ texture, i, t };
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

		auto quads = quad_buffer{ usage };
		quads.reserve(size(map));

		for (const auto &t : map)
		{
			//change array once we've added every tile with this texture
			if (t.texture != current_tex)
			{
				quads.apply();
				texture_layers.emplace_back(texture_layer{ current_tex, std::move(quads) });
				current_tex = t.texture;
				quads = quad_buffer{ usage };
				quads.reserve(size(map));
			}

			const auto pos = to_2d_index<tile_position>(t.index, width);
			const auto p = to_pixels(pos, tile_size);

			const auto quad_rect = rect_float{
				float_cast(p.x),
				float_cast(p.y),
				float_cast(tile_size),
				float_cast(tile_size)
			};

			const auto text_rect = rect_float{
				float_cast(t.tile.left),
				float_cast(t.tile.top),
				float_cast(tile_size),
				float_cast(tile_size)
			};

			//set vertex position and tex coordinates
			quads.append(make_quad_animation(quad_rect, text_rect));
		}

		quads.apply();
		texture_layers.emplace_back(texture_layer{ current_tex, std::move(quads) });
	}

	//=========================================//
	//				mutable_tile_map		   //
	//=========================================//

	constexpr auto vertex_usage = sf::VertexBuffer::Usage::Static;

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
		
		const auto map_end = integer_cast<tile_index_t>(size(map.tiles));
		for (auto i = tile_index_t{}; i < map_end; ++i)
		{
			if (map.tiles[i] != _tiles.tiles[i])
			{
				const auto &t = get_tile(map, map.tiles[i]);
				const auto pos = to_2d_index<tile_position>(i, map.width);
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
		
		const auto level_y = integer_cast<tile_index_t>(_tiles.tiles.size() / _tiles.width);
		const auto level_x = integer_cast<tile_index_t>(_tiles.width);
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

	void mutable_tile_map::_update_tile(const tile_position p, const resources::tile &t)
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
				layer = &texture_layers.emplace_back(
					texture_layer{ t.texture, { vertex_usage } }
				);
			}
		}

		//find the location of the current tile
		texture_layer *current_layer = nullptr;

		const auto tile_size = resources::get_tile_size();
		const auto pixel_pos = to_pixels(p, tile_size);
		
		for (auto &l : texture_layers)
		{
			const auto buffer_size = std::size(l.quads);
			for (auto i = std::size_t{}; i < buffer_size; ++i)
			{
				const auto vertex = l.quads.get_quad(i);
				const auto pos = vector_int{
					integral_cast<vector_int::value_type>(vertex[i].position.x),
					integral_cast<vector_int::value_type>(vertex[i].position.y)
				};

				if (pos == pixel_pos)
				{
					//if the arrays are the same, then replace the quad
					//otherwise remove the quad from the old array and insert into the new one
					if (layer == current_layer)
					{
						if (layer) // IF !LAYER, then both layers are null, just skip
							_replace_tile(*layer, i, pixel_pos, tile_size, t);
					}
					else if (current_layer)
					{
						_remove_tile(*current_layer, i);
						if (layer)//only write into a new layer if we have one
							_add_tile(*layer, pixel_pos, tile_size, t);
						else
						{
							//remove layer if its empty
							if (std::size(current_layer->quads) == 0)
								_remove_layer(current_layer->texture);
						}
					}
					else // tile isn't currently in an array, just add the tile
					{
						_add_tile(*layer, pixel_pos, tile_size, t);
					}

					return;
				}
			} // !for (auto i)
		} // !for(auto &l)
	}

	void mutable_tile_map::_remove_tile(texture_layer &l, const std::size_t pos)
	{
		l.quads.swap(pos, std::size(l.quads));
		l.quads.pop_back();
		return;
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

	void mutable_tile_map::_replace_tile(texture_layer &l, const std::size_t i,
		const tile_position pos, const resources::tile_size_t tile_size, const resources::tile &t)
	{
		_remove_tile(l, i);
		_add_tile(l, pos, tile_size, t);
	}

	void mutable_tile_map::_add_tile(texture_layer &l, const tile_position pos, 
		const resources::tile_size_t tile_size, const resources::tile &t)
	{
		const auto quad_rect = rect_float{
			float_cast(pos.x * tile_size),
			float_cast(pos.y * tile_size),
			float_cast(tile_size),
			float_cast(tile_size)
		};

		const auto tex_rect = rect_float{
			float_cast(t.left),
			float_cast(t.top),
			float_cast(tile_size),
			float_cast(tile_size)
		};

		l.quads.append(make_quad_animation(quad_rect, tex_rect));
	}
}
