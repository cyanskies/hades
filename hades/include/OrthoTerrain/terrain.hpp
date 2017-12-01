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

namespace ortho_terrain
{
	using tile_count_t = hades::types::uint32;

	//an array of tiles, can be converted into a tilemap to draw
	using TileArray = std::vector<tile>;
	using VertexArray = std::vector<sf::Vertex>;
	//terrain vertex is a vector of verticies, each row is 1 element longer than the equivalent tile array
	//and contains one extra row
	using TerrainVertex = std::vector<const resources::terrain*>;

	using MapData = std::tuple<TileArray, tile_count_t>;
	//uses the transition data to convert terrain verticies into an array of tiles
	//returns the tile array and the number of tiles per row.
	MapData ConvertToTiles(const TerrainVertex&, tile_count_t vertexPerRow);

	using VertexData = std::tuple<TerrainVertex, tile_count_t>;
	VertexData ConvertToVertex(const MapData&);

	//thrown by functions in the ortho-terrain namespace
	//for unrecoverable errors
	class exception : public std::exception
	{
	public:
		using std::exception::exception;
		using std::exception::what;
	};

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
		using vArray = std::pair<hades::resources::texture*, VertexArray>;
		std::vector<vArray> Chunks;
	};

	//a drawable map of terrain tiles
	class EditMap : public TileMap
	{
	public:
		EditMap() = default;
		EditMap(const MapData&);

		void create(const MapData&) override;
		//draw over a tile
		//amount is the number of rows of adjacent tiles to replace as well.
		//eg amount = 1, draws over 9 tiles worth,
		void replace(const tile&, const sf::Vector2u &position, hades::types::uint8 amount = 0, bool updateVertex = false);
		//draw over a vertex with transitions fixup
		//amount is the number of rows of adjacent vertex to replace as well.
		//eg amount = 0, changes 4 tiles worth,
		void replace(const resources::terrain&, const sf::Vector2u &position, hades::types::uint8 amount = 0);
		
		MapData getMap() const;

	private:
		//fixes transitions for vertex in an area 
		void _cleanTransitions(const sf::Vector2u &position, sf::Vector2u size = { 0,0 });

		void _replaceTile(VertexArray &a, const sf::Vector2u &position, const tile& t);
		void _removeTile(VertexArray &a, const sf::Vector2u &position);
		void _addTile(VertexArray &a, const sf::Vector2u &position, const tile& t);

		tile_size_t _tile_size;
		tile_count_t _width;
		TileArray _tiles;
		tile_count_t _vertex_width;
		TerrainVertex _vertex;
	};
}

#endif // !ORTHO_TERRAIN_HPP
