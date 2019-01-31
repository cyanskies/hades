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

	//TODO: split chunks based on visible area
	//a class for rendering MapData as a tile map
	class TileMap : public sf::Drawable
	{
	public:
		TileMap() = default;
		TileMap(const MapData&);

		virtual ~TileMap() {}

		virtual void create(const MapData&);

		void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

		sf::FloatRect getLocalBounds() const;

	protected:
		using vArray = std::pair<const hades::resources::texture*, VertexArray>;
		std::vector<vArray> Chunks;
	};

	//an expanded upon TileMap, that allows changing the map on the fly
	//mostly used for editors
	class MutableTileMap final : public TileMap
	{
	public:
		MutableTileMap() = default;
		MutableTileMap(const MapData&);

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
