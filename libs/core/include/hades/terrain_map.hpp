#ifndef HADES_TERRAIN_MAP_HPP
#define HADES_TERRAIN_MAP_HPP

#include "SFML/Graphics/Drawable.hpp"

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

		void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

		rect_float get_local_bounds() const;

	private:
		immutable_tile_map _tile_layer;
		std::vector<immutable_tile_map> _terrain_layers;
	};

	class mutable_terrain_map  final : public sf::Drawable
	{
	public:
		mutable_terrain_map() = default;
		mutable_terrain_map(const mutable_terrain_map&) = default;
		mutable_terrain_map(const terrain_map&);

		void create(const terrain_map&);
		void update(const terrain_map&);

		void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

		rect_float get_local_bounds() const;

		void place_tile(const std::vector<tile_position>&, const resources::tile&);
		void place_terrain(const std::vector<terrain_vertex_position>&, const resources::terrain*);

		terrain_map get_map() const;

	private:
		terrain_map _map;
		mutable_tile_map _tile_layer;
		std::vector<mutable_tile_map> _terrain_layers;
	};
}

#endif // !HADES_TERRAIN_MAP_HPP