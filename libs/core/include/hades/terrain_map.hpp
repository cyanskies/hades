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

		void draw_depth_buffer(bool b) noexcept
		{
			_debug_depth = b;
			return;
		}

		bool draw_depth_buffer() noexcept
		{
			return _debug_depth;
		}

		void show_grid(bool b) noexcept
		{
			_show_grid = b;
			return;
		}

		bool show_grid() const noexcept
		{
			return _show_grid;
		}

		void set_world_rotation(float degrees);
		void set_sun_angle(float degrees);

		void place_tile(tile_position, const resources::tile&);
		void place_terrain(terrain_vertex_position, const resources::terrain*);

		void raise_terrain(terrain_vertex_position, uint8_t amount);
		void lower_terrain(terrain_vertex_position, uint8_t amount);

		//void create_cliff(terrain_vertex_position, terrain_vertex_position)

		void replace_heightmap(const std::vector<std::uint8_t>& height) noexcept
		{
			assert(size(height) == size(_map.heightmap));
			_map.heightmap = height;
			return;
		}

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
		terrain_map _map;
		quad_buffer _quads;
		map_info _info;
		resources::shader_proxy _shader;
		resources::shader_proxy _shader_debug_depth;
		resources::shader_proxy _shader_no_height;
		resources::shader_proxy _shader_no_height_debug_depth;
		//resources::shader_uniform_map _uniforms;
		rect_float _local_bounds = {};
		const resources::terrain_settings* _settings = {};
		const resources::texture* _grid_tex = {};
		std::size_t _grid_start = {};
		float _sun_angle = 135.f;
		bool _show_grid = false;
		bool _needs_apply = false;
		bool _show_height = true;
		bool _debug_depth = false;
	};
}

#endif // !HADES_TERRAIN_MAP_HPP
