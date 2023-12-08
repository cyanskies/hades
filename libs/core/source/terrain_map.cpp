#include "hades/terrain_map.hpp"

#include "SFML/Graphics/RenderTarget.hpp"

namespace hades
{
	void register_terrain_map_resources(data::data_manager &d)
	{
		register_texture_resource(d);
		register_terrain_resources(d, resources::texture_functions::make_resource_link);
	}

	immutable_terrain_map::immutable_terrain_map(const terrain_map &t) :
		_tile_layer{t.tile_layer}
	{
		for (const auto &l : t.terrain_layers)
			_terrain_layers.emplace_back(l);
	}

	void immutable_terrain_map::create(const terrain_map &t)
	{
		_tile_layer.create(t.tile_layer);
		_terrain_layers.clear();
		for (const auto &l : t.terrain_layers)
			_terrain_layers.emplace_back(l);
	}

	void immutable_terrain_map::draw(sf::RenderTarget &t, const sf::RenderStates& s) const
	{
		for (auto iter = std::rbegin(_terrain_layers);
			iter != std::rend(_terrain_layers); ++iter)
			t.draw(*iter, s);

		t.draw(_tile_layer, s);
	}

	rect_float immutable_terrain_map::get_local_bounds() const noexcept
	{
		return _tile_layer.get_local_bounds();
	}

	//==================================//
	//		mutable_terrain_map			//
	//==================================//

	// TODO: add height, usage can be hardcoded
	static mutable_terrain_map::world_layer create(const tile_map& map_data, const resources::tile_size_t tile_size)
	{
		constexpr auto usage = sf::VertexBuffer::Usage::Dynamic;

		const auto& tiles = map_data.tiles;
		const auto width = unsigned_cast(map_data.width);

		auto w = mutable_terrain_map::world_layer{};

		//generate a drawable array
		w.tex_layers.clear();

		struct map_tile {
			const resources::texture* texture = nullptr;
			tile_index_t index;
			resources::tile tile;
		};

		//collect info about each cell in the map
		std::vector<map_tile> map;
		std::vector<const hades::resources::texture*> tex_cache;
		const auto map_end = size(tiles);
		for (auto i = std::size_t{}; i < map_end; ++i)
		{
			const auto t = get_tile(map_data, tiles[i]);

			//skip 'empty' tiles, no need to render a transparent texture
			if (!t.tex)
				continue;

			const hades::resources::texture* texture = nullptr;

			for (auto& c : tex_cache)
			{
				if (c == t.tex.get())
					texture = c;
			}

			if (!texture)
			{
				texture = t.tex.get();
				tex_cache.push_back(texture);
			}

			assert(texture);
			const map_tile ntile{ texture, integer_cast<tile_index_t>(i), t };
			map.emplace_back(ntile);
		}

		//sort the tiles by texture;
		std::sort(map.begin(), map.end(), [](const auto& lhs, const auto& rhs) {
			constexpr auto less = std::less<const resources::texture*>{};
			return less(lhs.texture, rhs.texture);
			});

		//whole map is transparent
		//exit early
		if (map.empty())
			return;

		auto current_tex = map.front().texture;
		auto quads = quad_buffer{ usage };
		quads.reserve(size(map));

		for (const auto& t : map)
		{
			//change array once we've added every tile with this texture
			if (t.texture != current_tex)
			{
				quads.apply();
				w.tex_layers.emplace_back(mutable_terrain_map::texture_layer{ current_tex, std::move(quads) });
				current_tex = t.texture;
				quads = quad_buffer{ usage };
				quads.reserve(size(map));
			}

			const auto pos = to_2d_index<tile_position>(t.index, map_data.width);
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

		w.tex_layers.emplace_back(mutable_terrain_map::texture_layer{ current_tex, std::move(quads) });

		for (auto& tex_layer : w.tex_layers)
		{
			tex_layer.quads.shrink_to_fit();
			tex_layer.quads.apply();
		}

		w.dirty_buffers = std::vector(size(w.tex_layers), false);
		return w;
	}

	static void draw(sf::RenderTarget& target, const mutable_terrain_map::world_layer& w, sf::RenderStates states)
	{
		for (const auto& layer : w.tex_layers)
		{
			states.texture = &resources::texture_functions::get_sf_texture(layer.texture);
			target.draw(layer.quads, states);
		}
	}

	mutable_terrain_map::mutable_terrain_map(terrain_map t)
	{
		create(std::move(t));
	}

	void mutable_terrain_map::create(terrain_map t)
	{
		_map = std::move(t);
		const auto t_size = resources::get_tile_size();
		_tile_layer = hades::create(_map.tile_layer, t_size);
		_terrain_layers.clear();
		for (const auto &l : _map.terrain_layers)
			_terrain_layers.emplace_back(hades::create(l, t_size));

		// TODO: create cliffs

		_local_bounds = rect_float{
			0.f, 0.f,
			float_cast(_map.tile_layer.width * t_size),
			float_cast((_map.tile_layer.tiles.size() / _map.tile_layer.width) * t_size)
		};

		return;
	}

	void mutable_terrain_map::update(terrain_map t)
	{
		create(t); // temp; probably too slow for gameplay

		// TODO: generate new map using the old map as a reference to only update changed sections
	}

	void mutable_terrain_map::draw(sf::RenderTarget &t, const sf::RenderStates& s) const
	{
		const auto layers = std::views::reverse(_terrain_layers);
		for (auto& l : layers)
			hades::draw(t, l, s);

		// TODO: draw cliffs

		hades::draw(t, _tile_layer, s);
	}

	rect_float mutable_terrain_map::get_local_bounds() const noexcept
	{
		return _local_bounds;
	}

	void mutable_terrain_map::place_tile(const tile_position p, const resources::tile &t)
	{
		hades::place_tile(_map, p, t);
	}

	void mutable_terrain_map::place_terrain(const terrain_vertex_position p, const resources::terrain *t)
	{
		hades::place_terrain(_map, p, t);		
	}

	void mutable_terrain_map::generate_layers()
	{
		create(_map); //temp

		// TODO: generate new map using the old map as a reference to only update changed sections
	}

	const terrain_map &mutable_terrain_map::get_map() const noexcept
	{
		return _map;
	}
}
