#ifndef ORTHO_SERIALISE_HPP
#define ORTHO_SERIALISE_HPP

#include "Hades/Types.hpp"

#include "Tiles/tiles.hpp"

namespace YAML
{
	class Emitter;
	class Node;
}

//contains serialisation functions for the tile maps
//tile map data can be stored in any yaml based save files
//under the tile-map: header
namespace tiles
{
	using version_t = hades::types::uint16;
	const version_t max_supported_version = 0;

	struct MapSaveData
	{
		//vector of tiles that make up the terrain
		std::vector<tile_count_t> tiles;
		//the width of the terrain rectangle
		tile_count_t map_width;
		//a tileset name, and the id of the first tile in that set
		using Tileset = std::tuple<hades::types::string, hades::types::uint32>;
		using TilesetList = std::vector<Tileset>;
		TilesetList tilesets;
	};

	//converts map data to an writable savedata
	MapSaveData CreateMapSaveData(const MapData& terrain);
	//converts mapsavedata to the renderable mapdata
	MapData CreateMapData(const MapSaveData& save);

	//throws tile_map_exception
	//capable of parsing any map file with a version lower than the supported level
	MapSaveData Load(const YAML::Node &root);

	//save always writes the format with max_supported_version
	YAML::Emitter &Save(const MapSaveData& obj, YAML::Emitter &target);
	YAML::Emitter &operator<<(YAML::Emitter &lhs, const MapSaveData &rhs);
}

#endif // !ORTHO_SERIALISE_HPP
