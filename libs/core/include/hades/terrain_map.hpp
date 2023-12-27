#ifndef HADES_TERRAIN_MAP_HPP
#define HADES_TERRAIN_MAP_HPP

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/RenderStates.hpp"

#include "hades/terrain.hpp"
#include "hades/tile_map.hpp"

namespace hades
{
	void register_terrain_map_resources(data::data_manager&);

	class immutable_terrain_map  final : public sf::Drawable
	{
	public:
		immutable_terrain_map() = default;
		immutable_terrain_map(const immutable_terrain_map&) = default;
		immutable_terrain_map(const terrain_map&);

		void create(const terrain_map&);

		void draw(sf::RenderTarget& target, const sf::RenderStates& states) const override;

		rect_float get_local_bounds() const noexcept;

	private:
		immutable_tile_map _tile_layer;
		std::vector<immutable_tile_map> _terrain_layers;
	};

	class mutable_terrain_map final : public sf::Drawable
	{
	public:
		mutable_terrain_map();
		mutable_terrain_map(const mutable_terrain_map&) = default;
		mutable_terrain_map(terrain_map);

		void reset(terrain_map); // equiv to creating a new object of this class, except we retain our allocated memory

		void set_height_enabled(bool b) noexcept
		{
			_show_height = b;
			return;
		}

		void place_tile(const tile_position, const resources::tile&);
		void place_terrain(const terrain_vertex_position, const resources::terrain*);

		void apply();
		void draw(sf::RenderTarget& target, const sf::RenderStates& states) const override;

		rect_float get_local_bounds() const noexcept
		{
			return _local_bounds;
		}

		const terrain_map& get_map() const noexcept
		{
			return _map;
		}

		bool is_height_enabled() const noexcept
		{
			return _show_height;
		}

	public:
		// internal structures
		struct map_tile
		{
			tile_index_t index; 
			std::array<std::uint8_t, 4> height = {};
			resources::tile_size_t left;
			resources::tile_size_t top;
			std::uint8_t texture = {};
			std::uint8_t layer = {};
		};

		struct map_info
		{
			std::vector<map_tile> tile_info;
			std::vector<resources::resource_link<resources::texture>> texture_table;
		};

	private:
		map_info _info;
		quad_buffer _quads;
		bool _needs_apply = false;
		bool _show_height = false;
		rect_float _local_bounds = {};
		terrain_map _map;
		resources::shader_uniform_map _uniforms;
		resources::shader_proxy _shader;
		resources::shader_proxy _shader_no_height;
	};
}

#endif // !HADES_TERRAIN_MAP_HPP
