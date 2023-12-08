#ifndef HADES_TERRAIN_MAP_HPP
#define HADES_TERRAIN_MAP_HPP

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/RenderStates.hpp"

#include "hades/terrain.hpp"
#include "hades/tile_map.hpp"

namespace hades
{
	void register_terrain_map_resources(data::data_manager&);

	class [[deprecated]] immutable_terrain_map  final : public sf::Drawable
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

	class mutable_terrain_map  final : public sf::Drawable
	{
	public:
		mutable_terrain_map() = default;
		mutable_terrain_map(const mutable_terrain_map&) = default;
		mutable_terrain_map(terrain_map);

		void create(terrain_map);
		void update(terrain_map);

		void draw(sf::RenderTarget& target, const sf::RenderStates& states) const override;

		rect_float get_local_bounds() const noexcept;

		// TODO: split this into place_tile/terrain and generate layers
		//		this will allow us to use the non-allocating tile_position functions
		void place_tile(tile_position, const resources::tile&);
		void place_terrain(terrain_vertex_position, const resources::terrain*);
		void raise_height(terrain_vertex_position, std::uint8_t);
		void lower_height(terrain_vertex_position, std::uint8_t);

		void generate_layers();

		const terrain_map &get_map() const noexcept;

	public:
		struct texture_layer
		{
			const resources::texture* texture = nullptr;
			quad_buffer quads;
		};

		struct world_layer
		{
			std::vector<texture_layer> tex_layers;
			std::vector<bool> dirty_buffers;
		};

	private:

		static world_layer _create_layer(const tile_map&, std::vector<std::uint8_t>);

		terrain_map _map;
		world_layer _tile_layer;
		std::vector<world_layer> _terrain_layers;

		rect_float _local_bounds;

		//mutable_tile_map _tile_layer;
		//std::vector<mutable_tile_map> _terrain_layers;

		// TODO: local_bounds
		// TODO: quad_buffer for cliff verts
	};
}

#endif // !HADES_TERRAIN_MAP_HPP
