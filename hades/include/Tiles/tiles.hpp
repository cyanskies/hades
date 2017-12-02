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

	//an array of tiles, can be converted into a tilemap to draw
	using TileArray = std::vector<tile>;
	using VertexArray = std::vector<sf::Vertex>;
	using MapData = std::tuple<TileArray, tile_count_t>;

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
