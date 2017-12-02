#ifndef TILES_RESOURCES_HPP
#define TILES_RESOURCES_HPP

#include "Hades/DataManager.hpp"

namespace tiles
{
	//registers the resources needed to use tilemaps and the tile editor
	void RegisterTileResources(hades::data::data_manager*);

	//tile sizes are capped by the data type used for texture sizes
	using tile_size_t = hades::resources::texture::size_type;

	//data needed to render a tile
	struct tile
	{
		hades::data::UniqueId texture = hades::data::UniqueId::Zero;
		tile_size_t left, top;
	};

	bool operator==(const tile &lhs, const tile &rhs);
	bool operator!=(const tile &lhs, const tile &rhs);

	namespace resources
	{
		void parseTileSettings(hades::data::UniqueId mod, YAML::Node& node, hades::data::data_manager*);
		void parseTileset(hades::data::UniqueId mod, YAML::Node& node, hades::data::data_manager*);

		const hades::types::string tile_settings_name = "tile-settings";
		extern std::vector<hades::data::UniqueId> Tilesets;

		struct tile_settings_t {};

		struct tile_settings : public::hades::resources::resource_type<tile_settings_t>
		{
			hades::resources::texture::size_type tile_size;
		};

		struct tileset_t {};

		//contains all the tiles defined by a particular tileset
		//used to map the integers in maps to actual tiles
		struct tileset : public hades::resources::resource_type<tileset_t>
		{
			std::vector<tile> tiles;
		};
	}
}

#endif // !TILES_RESOURCES_HPP
