#ifndef ORTHO_TERRAIN_HPP
#define ORTHO_TERRAIN_HPP

#include <optional>
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
	using vertex_count_t = tiles::tile_count_t;
	TerrainVertex AsTerrainVertex(const std::vector<tiles::TileArray> &map, const std::vector<const resources::terrain*> &terrainset, tiles::tile_count_t width);
	std::tuple<vertex_count_t, vertex_count_t> CalculateVertCount(std::tuple<tiles::tile_count_t, tiles::tile_count_t> count);
	const resources::terrain *FindTerrain(const tiles::TileArray &layer, const std::vector<const resources::terrain*> &terrain_set);

	struct TerrainMapData
	{
		//terain_set.size() == tile_map_stack.size()
		//terrain_set implicitly has a empty layer at the front
		std::vector<const resources::terrain*> terrain_set;
		//terrain maps are stored as a variable sized stack of tile layers
		std::vector<tiles::TileArray> tile_map_stack;
	};

	void ReplaceTerrain(TerrainMapData &map, TerrainVertex &verts, tiles::tile_count_t width, const resources::terrain *t, sf::Vector2i pos, tiles::draw_size_t size);

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
		TerrainMap(const TerrainMapData&, tiles::tile_count_t width);

		void create(const TerrainMapData&, tiles::tile_count_t width);

		void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

		sf::FloatRect getLocalBounds() const;

	protected:
		std::vector<tiles::TileMap> Map;
	};

	class MutableTerrainMap : public sf::Drawable, public sf::Transformable
	{
	public:
		MutableTerrainMap() = default;
		MutableTerrainMap(const TerrainMapData&, tiles::tile_count_t width);

		//loads a map
		void create(const TerrainMapData&, tiles::tile_count_t width);
		//creates a new black map with the specified settings
		void create(std::vector<const resources::terrain*> tset, tiles::tile_count_t width, tiles::tile_count_t height);

		void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

		sf::FloatRect getLocalBounds() const;

		//draw over a tile
		//amount is the number of rows of adjacent tiles to replace as well.
		//eg amount = 1, draws over 9 tiles worth,
		void replace(const resources::terrain *t, const sf::Vector2i &position, tiles::draw_size_t amount = 0);

		void setColour(sf::Color c);

		TerrainMapData getMap() const;

	private:
		std::vector<tiles::MutableTileMap> _terrain_layers;
		using tile_layer = std::tuple<const resources::terrain*, tiles::TileArray>;
		std::vector<tile_layer> _tile_layers;
		tiles::tile_count_t _width = 0u;
		sf::Color _colour = sf::Color::White;
		TerrainVertex _vdata;
	};

	struct level : public tiles::level
	{
		TerrainMapData terrain;
	};
}

#endif // !ORTHO_TERRAIN_HPP
