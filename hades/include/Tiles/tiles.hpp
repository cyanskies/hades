#ifndef TILES_TILES_HPP
#define TILES_TILES_HPP

#include <vector>

#include "SFML/Graphics/Vertex.hpp"

#include "Hades/Types.hpp"

#include "Tiles/resources.hpp"

namespace tiles
{
	//tile_count_t::max is the largest amount of tiles that can be in a tileset, or map
	using tile_count_t = hades::types::uint32;

	//raw map data, is a stream of tile_count id's
	//and a map of tilesets along with thier first id's
	using TileSetInfo = std::tuple<hades::data::UniqueId, tile_count_t>;
	using RawMap = std::tuple<std::vector<TileSetInfo>, std::vector<tile_count_t>>;

	//an array of tiles, can be converted into a tilemap to draw
	using TileArray = std::vector<tile>;
	using VertexArray = std::vector<sf::Vertex>;
	using MapData = std::tuple<TileArray, tile_count_t>;

	//converts a raw map into mapdata
	MapData as_mapdata(const RawMap& map);
	//converts mapdata back into a raw map
	RawMap as_rawmap(const MapData & map);

	//thrown by tile maps for unrecoverable errors
	class tile_map_exception : public std::exception
	{
	public:
		using std::exception::exception;
		using std::exception::what;
	};

	//a class for rendering MapData as a tile map
	class TileMap;
	//an expanded upon TileMap, that allows changing the map on the fly
	class MutableTileMap;
}

#endif // !TILES_TILES_HPP
