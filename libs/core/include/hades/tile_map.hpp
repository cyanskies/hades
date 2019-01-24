#ifndef HADES_TILE_MAP_HPP
#define HADES_TILE_MAP_HPP

#include <vector>

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/Transformable.hpp"
#include "SFML/Graphics/Vertex.hpp"

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
	//registers the resources needed to use tilemaps and the tile editor
	void register_tiles(data::data_manager&);
	
	//tile sizes are capped by the data type used for texture sizes
	using tile_size_t = hades::resources::texture::size_type;
	//TODO: rename to tag_list
	// this should be defined elsewhere, not in tiles
	using traits_list = std::vector<hades::unique_id>;

	//data needed to render and interact with a tile
	struct tile
	{
		const hades::resources::texture *texture = nullptr;
		tile_size_t left = 0, top = 0;
		traits_list traits;
	};

	bool operator==(const tile &lhs, const tile &rhs);
	bool operator!=(const tile &lhs, const tile &rhs);
	bool operator<(const tile &lhs, const tile &rhs);

	namespace resources
	{
		void parseTileSettings(hades::unique_id, const YAML::Node&, hades::data::data_manager*);
		std::vector<tile> ParseTileSection(hades::unique_id texture, tile_size_t tile_size, YAML::Node &tiles_node,
			hades::types::string resource_type, hades::types::string name, hades::unique_id mod);

		extern std::vector<hades::unique_id> Tilesets;

		struct tileset_t {};

		//contains all the tiles defined by a particular tileset
		//used to map the integers in maps to actual tiles
		struct tileset : public hades::resources::resource_type<tileset_t>
		{
			tileset();
			tileset(hades::resources::resource_type<tileset_t>::loaderFunc func);
			std::vector<tile> tiles;
		};

		struct tile_settings_t {};

		struct tile_settings : public::hades::resources::resource_type<tile_settings_t>
		{
			tile_size_t tile_size = 0;
			const tileset *error_tileset = nullptr;
			const tileset *empty_tileset = nullptr;
		};
	}

	//tile_count_t::max is the largest amount of tiles that can be in a tileset, or map
	using tile_count_t = std::size_t;
	//draw size is used for measuring brush sizes
	using draw_size_t = hades::types::int8;

	std::tuple<tile_count_t, tile_count_t> CalculateTileCount(std::tuple<hades::level_size_t, hades::level_size_t> size, tile_size_t tile_size);
	//raw map data, is a stream of tile_count id's
	//and a map of tilesets along with thier first id's
	// and also a width
	// raw maps can be stored and transfered
	using TileSetInfo = std::tuple<hades::unique_id, tile_count_t>;
	//							tilesets used in the map, the actual tile id map data, map width
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

	tile_count_t FlatPosition(const sf::Vector2u &position, tile_count_t width);
	sf::Vector2u InflatePosition(tile_count_t i, tile_count_t width);

	using vertex_count_t = tile_count_t;
	vertex_count_t CalculateVertexCount(tile_count_t count);

	//thrown by tile maps for unrecoverable errors
	class tile_map_exception : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	//TODO: split chunks based on visible area
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
	//mostly used for editors
	class MutableTileMap : public TileMap
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

	//throws tile_map_exception if the settings cannot be retrieved for any reason.
	const resources::tile_settings *GetTileSettings();
	const TileArray &GetErrorTileset();
	tile GetErrorTile();
	tile GetEmptyTile();

	std::vector<sf::Vector2i> AllPositions(const sf::Vector2i &position, tiles::draw_size_t amount);

	struct tile_layer
	{
		//tilesets
		std::vector<std::tuple<hades::types::string, tile_count_t>> tilesets;
		//tile data
		std::vector<tile_count_t> tiles;
	};

	////reads tiles from the yaml node and stores it in target
	//void ReadTileLayerFromYaml(const YAML::Node &n, tile_layer &target);
	//void ReadTilesFromYaml(const YAML::Node&, level &target);
	////does the reverse of the above function
	//YAML::Emitter &WriteTileLayerToYaml(const tile_layer &l, YAML::Emitter &e);
	//YAML::Emitter &WriteTilesToYaml(const level&, YAML::Emitter &);
}

#endif // !HADES_TILES_HPP
