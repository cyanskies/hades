#ifndef ORTHO_TERRAIN_HPP
#define ORTHO_TERRAIN_HPP

#include <vector>

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/Transformable.hpp"
#include "SFML/Graphics/VertexArray.hpp"

#include "Hades/QuadMap.hpp"
#include "Hades/Types.hpp"
#include "Hades/UniqueId.hpp"

#include "OrthoTerrain/resources.hpp"

#include "Tiles/tiles.hpp"

namespace ortho_terrain
{
	void EnableTerrain(hades::data::data_system *d);

	//terrain vertex is a vector of verticies, each row is 1 element longer than the equivalent tile array
	//and contains one extra row
	using TerrainVertex = std::vector<const resources::terrain*>;

	//uses the transition data to convert terrain verticies into an array of tiles
	//returns the tile array and the number of tiles per row.
	tiles::MapData ConvertToTiles(const TerrainVertex&, tiles::tile_count_t vertexPerRow);

	using VertexMapData = std::tuple<TerrainVertex, tiles::tile_count_t>;
	VertexMapData ConvertToVertex(const tiles::MapData&);

	//thrown by functions in the ortho-terrain namespace
	//for unrecoverable errors
	class exception : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	//a drawable map of terrain tiles
	class MutableTerrainMap : public tiles::MutableTileMap
	{
	public:
		MutableTerrainMap() = default;
		MutableTerrainMap(const tiles::MapData&);

		void create(const tiles::MapData&) override;
		//draw over a tile
		//amount is the number of rows of adjacent tiles to replace as well.
		//eg amount = 1, draws over 9 tiles worth,
		void replace(const tiles::tile&, const sf::Vector2i &position, tiles::draw_size_t amount = 0, bool updateVertex = false);
		//draw over a vertex with transitions fixup
		//amount is the number of rows of adjacent vertex to replace as well.
		//eg amount = 0, changes 4 tiles worth,
		void replace(const resources::terrain&, const sf::Vector2i &position, tiles::draw_size_t amount = 0);

	private:
		//fixes transitions for vertex in an area
		void _cleanTransitions(const sf::Vector2u &position, sf::Vector2u size = { 0,0 });

		tiles::tile_count_t _vertex_width;
		TerrainVertex _vertex;
	};
}

#endif // !ORTHO_TERRAIN_HPP
