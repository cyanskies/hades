#ifndef TILES_TILES_HPP
#define TILES_TILES_HPP

#include <vector>

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/Transformable.hpp"
#include "SFML/Graphics/Vertex.hpp"

#include "Hades/Types.hpp"

#include "Tiles/resources.hpp"

namespace tiles
{
	//tile_count_t::max is the largest amount of tiles that can be in a tileset, or map
	using tile_count_t = hades::types::uint32;

	//raw map data, is a stream of tile_count id's
	//and a map of tilesets along with thier first id's
	// and also a width
	using TileSetInfo = std::tuple<hades::data::UniqueId, tile_count_t>;
	using RawMap = std::tuple<std::vector<TileSetInfo>, std::vector<tile_count_t>, tile_count_t>;

	//an array of tiles, can be converted into a tilemap to draw
	using TileArray = std::vector<tile>;
	using VertexArray = std::vector<sf::Vertex>;
	using MapData = std::tuple<TileArray, tile_count_t>;

	//converts a raw map into mapdata
	MapData as_mapdata(const RawMap& map);
	//converts mapdata back into a raw map
	RawMap as_rawmap(const MapData & map);

	//converts tile positions in the flat map to a 2d position on the screen
	//NOTE: this returns a pixel position with the maps origin in the top left corner.
	std::tuple<hades::types::int32, hades::types::int32> GetGridPosition(hades::types::uint32 tile_number,
		hades::types::uint32 tiles_per_row, hades::types::uint32 tile_size);

	//thrown by tile maps for unrecoverable errors
	class tile_map_exception : public std::exception
	{
	public:
		using std::exception::exception;
		using std::exception::what;
	};

	//a class for rendering MapData as a tile map
	class TileMap : public sf::Drawable, public sf::Transformable
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
	class MutableTileMap : public TileMap
	{
	public:
		MutableTileMap() = default;
		MutableTileMap(const MapData&);

		void create(const MapData&) override;
		//draw over a tile
		//amount is the number of rows of adjacent tiles to replace as well.
		//eg amount = 1, draws over 9 tiles worth,
		void replace(const tile&, const sf::Vector2u &position, hades::types::uint8 amount = 0);

		MapData getMap() const;

	private:
		void _replaceTile(VertexArray &a, const sf::Vector2u &position, const tile& t);
		void _removeTile(VertexArray &a, const sf::Vector2u &position);
		void _addTile(VertexArray &a, const sf::Vector2u &position, const tile& t);

		tile_size_t _tile_size;
		tile_count_t _width;
		TileArray _tiles;
		tile_count_t _vertex_width;
	};
	//throws tile_map_exception if the settings cannot be retrieved for any reason.
	const resources::tile_settings &GetTileSettings();
	const TileArray &GetErrorTileset();
	tile GetErrorTile();
}

#endif // !TILES_TILES_HPP
