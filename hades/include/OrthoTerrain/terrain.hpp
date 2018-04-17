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

	struct TerrainMapData
	{
		//terrain maps are stored as a variable sized stack of tile layers
		//each layer has a vertex map and a tile map
		//the vertex map is only used in the case of supporting a mutable terrain map
		//the top of the stack is represented by a regular tilemap 
		tiles::tile_count_t tile_count, map_width;
		TerrainVertex terrain_vertex;
		std::vector<tiles::TileArray> tile_map_stack;
	};

	//thrown by functions in the ortho-terrain namespace
	//for unrecoverable errors
	class exception : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	class TerrainMap : public sf::Drawable, public sf::Transformable
	{
	public:
		TerrainMap() = default;
		TerrainMap(const TerrainMapData&);

		void create(const TerrainMapData&);

		void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

		sf::FloatRect getLocalBounds() const;

	private:
		std::vector<tiles::TileMap> _map;
	};

	class MutableTerrainMap : public sf::Drawable, public sf::Transformable
	{
	public:
		MutableTerrainMap() = default;
		MutableTerrainMap(const TerrainMapData&);

		void create(const TerrainMapData&);

		void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

		sf::FloatRect getLocalBounds() const;
		//draw over a tile
		//amount is the number of rows of adjacent tiles to replace as well.
		//eg amount = 1, draws over 9 tiles worth,
		void replace(const resources::terrain *t, const sf::Vector2i &position, tiles::draw_size_t amount = 0);

		void setColour(sf::Color c);

		TerrainMapData getMap() const;
	private:
		using terrain_layer = std::tuple<const resources::terrain*, tiles::MutableTileMap>;
		
		std::vector<terrain_layer> _terrain_layers;
		TerrainVertex _vdata;
	};

	struct level : public objects::level
	{

	};
}

#endif // !ORTHO_TERRAIN_HPP
