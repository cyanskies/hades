#ifndef ORTHO_SERIALISE_HPP
#define ORTHO_SERIALISE_HPP

#include "Hades/Types.hpp"

#include "OrthoTerrain/terrain.hpp"

namespace YAML
{
	class Emitter;
	class Node;
}

//contains serialisation functions for the ortho terrain maps
namespace ortho_terrain
{
	using version_t = hades::types::uint16;
	const version_t max_supported_version = 0;

	struct OrthoSave
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

	//converts map data to an orthosave
	OrthoSave CreateOrthoSave(const MapData& terrain);
	MapData CreateMapData(const OrthoSave& save);

	//throws ortho_terrain::exception
	//capable of parsing any map file with a version lower than the supported level
	OrthoSave Load(const YAML::Node &root);

	//save always writes the format with max_supported_version
	YAML::Emitter &Save(const OrthoSave& obj, YAML::Emitter &target);
	YAML::Emitter &operator<<(YAML::Emitter &lhs, const OrthoSave &rhs);
}

#endif // !ORTHO_SERIALISE_HPP
