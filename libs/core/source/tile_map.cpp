#include "hades/tile_map.hpp"

#include <array>

#include "SFML/System/Err.hpp"
#include "SFML/Graphics/Image.hpp"
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
		register_tiles_resources(d, tex_funcs::get_resource);

		//add texture to error tile
		//const auto settings = resources::get_tile_settings();
		//const auto &error_tile = resources::get_error_tile();
		const auto settings = 
			d.find_or_create<resources::tile_settings>(resources::get_tile_settings_id(), unique_id::zero);
		assert(settings);
		assert(settings->error_tileset);
		assert(!settings->error_tileset->tiles.empty());
		const auto &error_tile = settings->error_tileset->tiles[0];
		auto error_tex = tex_funcs::find_create_texture(d, error_tile.texture.id(), unique_id::zero);
		auto img = sf::Image{};
		img.create(1u, 1u, sf::Color::Magenta);
		auto& tex = tex_funcs::get_sf_texture(error_tex);
		
		auto sb = std::stringbuf{};
		const auto prev = sf::err().rdbuf(&sb);
		if(tex.loadFromImage(img))
			tex_funcs::set_settings(error_tex, { settings->tile_size, settings->tile_size }, 
			false, true, false, true);
		else
		{
			LOGERROR("Failed to generate error tile texture");
			LOGERROR(sb.str());
		}
		sf::err().set_rdbuf(prev);
		
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

	void immutable_tile_map::draw(sf::RenderTarget& target, const sf::RenderStates& s) const
	{
		auto states = s;
		for (const auto &layer : texture_layers)
		{
			states.texture = &tex_funcs::get_sf_texture(layer.texture);
			target.draw(layer.quads, states);
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
				if (c == t.texture.get())
					texture = c;
			}

			if (!texture)
			{
				texture = t.texture.get();
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

		texture_layers.emplace_back(texture_layer{ current_tex, std::move(quads) });

		for (auto& tex_layer : texture_layers)
		{
			tex_layer.quads.shrink_to_fit();
			tex_layer.quads.apply();
		}
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
		_dirty_buffers = std::vector(size(texture_layers), false);
	}

	void mutable_tile_map::update(const tile_map &map)
	{
		// TODO: it may just be faster to call create and overwrite everything
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
		_apply();
		return;
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

		_apply();
		return;
	}

	tile_map mutable_tile_map::get_map() const
	{
		return _tiles;
	}

	void mutable_tile_map::_apply()
	{
		// apply the quad buffers
		const auto count = size(texture_layers);
		assert(count == size(_dirty_buffers));

		for (auto i = std::size_t{}; i < count; ++i)
		{
			if (_dirty_buffers[i])
				texture_layers[i].quads.apply();
		}

		// clear all the dirty bits
		std::fill_n(begin(_dirty_buffers), count, false);
		return;
	}

	void mutable_tile_map::_update_tile(const tile_position p, const resources::tile &t)
	{
		constexpr auto bad_index = std::numeric_limits<std::size_t>::max();
		auto layer_index = bad_index;
		auto layer_count = size(texture_layers);

		if (t.texture)
		{
			for (auto i = std::size_t{}; i < layer_count; ++i)
			{
				if (texture_layers[i].texture == t.texture.get())
				{
					layer_index = i;
					break;
				}
			}

			//create a new vertex array if needed
			if (layer_index == bad_index)
			{
				layer_index = size(texture_layers);

				texture_layers.emplace_back(
					texture_layer{ t.texture.get(), {vertex_usage}}
				);
				_dirty_buffers.emplace_back(true);

				layer_count = layer_index + 1;
			}
		}

		//find the location of the current tile
		auto current_layer = bad_index;
		auto quad_index = bad_index;

		const auto tile_size = resources::get_tile_size();
		const auto pixel_pos = to_pixels(p, tile_size);
		
		for (auto i = std::size_t{}; i < layer_count; ++i)
		{
			auto& l = texture_layers[i];
			const auto buffer_size = std::size(l.quads);
			for (auto j = std::size_t{}; j < buffer_size; ++j)
			{
				const sf::Vertex vertex = l.quads.get_quad(j)[0];
				const auto pos = vector_int{
					integral_cast<vector_int::value_type>(vertex.position.x),
					integral_cast<vector_int::value_type>(vertex.position.y)
				};

				if (pos == pixel_pos)
				{
					current_layer = i;
					quad_index = j;
				}
			}
		}
		
		//if the arrays are the same, then replace the quad
		//otherwise remove the quad from the old array and insert into the new one
		if (layer_index == current_layer)
		{
			if (quad_index != bad_index) // IF !LAYER, then both layers are null, just skip
				_replace_tile(layer_index, quad_index, pixel_pos, tile_size, t);
		}
		else if (current_layer != bad_index)
		{
			_remove_tile(current_layer, quad_index);
			if (layer_index != bad_index) //only write into a new layer if we have one
				_add_tile(layer_index, pixel_pos, tile_size, t);
			else
			{
				// remove layer if its empty
				if (std::size(texture_layers[current_layer].quads) == 0)
					_remove_layer(current_layer);
			}
		}
		else // tile isn't currently in an array, just add the tile
		{
			_add_tile(layer_index, pixel_pos, tile_size, t);
		}

		return;
	}

	void mutable_tile_map::_remove_tile(const std::size_t i, const std::size_t pos)
	{
		auto& l = texture_layers[i];
		l.quads.swap(pos, std::size(l.quads) - 1);
		l.quads.pop_back();
		_dirty_buffers[i] = true;
		return;
	}

	void mutable_tile_map::_remove_layer(const std::size_t layer)
	{
		const auto iter = std::next(begin(texture_layers), layer);
		texture_layers.erase(iter);
		const auto iter2 = std::next(begin(_dirty_buffers), layer);
		_dirty_buffers.erase(iter2);
		return;
	}

	void mutable_tile_map::_replace_tile(const std::size_t layer, const std::size_t i,
		const tile_position pos, const resources::tile_size_t tile_size, const resources::tile &t)
	{
		_remove_tile(layer, i);
		_add_tile(layer, pos, tile_size, t);
	}

	void mutable_tile_map::_add_tile(const std::size_t i, const tile_position pixel_pos,
		const resources::tile_size_t tile_size, const resources::tile &t)
	{
		auto& l = texture_layers[i];

		const auto quad_rect = rect_float{
			float_cast(pixel_pos.x),
			float_cast(pixel_pos.y),
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
		_dirty_buffers[i] = true;
		return;
	}
}
