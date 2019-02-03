#ifndef HADES_TILE_MAP_HPP
#define HADES_TILE_MAP_HPP

#include <vector>

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/Vertex.hpp"

#include "hades/exceptions.hpp"
#include "hades/level.hpp"
#include "hades/texture.hpp"
#include "hades/tiles.hpp"
#include "hades/types.hpp"
#include "hades/vertex_buffer.hpp"

//adds support for drawing tile maps

namespace hades::data
{
	class data_system;
}

namespace hades
{
	//registers the resources needed to use tilemaps
	//same as the one in tiles.hpp but also registers the texture loading
	void register_tile_map_resources(data::data_manager&);

	//thrown by tile maps for unrecoverable errors
	class tile_map_error : public runtime_error
	{
	public:
		using runtime_error::runtime_error;
	};

	//an immutable type for drawing tile_map instances
	class immutable_tile_map : public sf::Drawable
	{
	public:
		immutable_tile_map() = default;
		immutable_tile_map(const tile_map&);

		virtual ~immutable_tile_map() {}

		virtual void create(const tile_map&);

		void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

		rect_float get_local_bounds() const;

	protected:
		struct texture_layer
		{
			const resources::texture *texture = nullptr;
			vertex_buffer vertex;
		};

		std::vector<texture_layer> texture_layers;
		rect_float local_bounds;
	};

	//an expanded upon immutable_tile_map, that allows changing the map on the fly
	//mostly used for editors
	class mutable_tile_map final : public immutable_tile_map
	{
	public:
		mutable_tile_map() = default;
		mutable_tile_map(const MapData&);

		void create(const MapData&) override;
		void update(const MapData&);
		//draw over a tile
		//amount is the number of rows of adjacent tiles to replace as well.
		//eg amount = 1, draws over 9 tiles worth,
		void replace(const tile&, const sf::Vector2i &position, draw_size_t amount = 0);

		void setColour(sf::Color c);

		MapData getMap() const;

	private:
		void _updateTile(const sf::Vector2u &position, const tile &t);
		void _replaceTile(VertexArray &a, const sf::Vector2u &position, const tile& t);
		void _removeTile(VertexArray &a, const sf::Vector2u &position);
		void _addTile(VertexArray &a, const sf::Vector2u &position, const tile& t);

		tile_size_t _tile_size;
		tile_count_t _width;
		TileArray _tiles;
		sf::Color _colour = sf::Color::White;
		tile_count_t _vertex_width;
	};
}

#endif // !HADES_TILES_HPP
