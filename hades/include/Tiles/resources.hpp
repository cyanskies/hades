#ifndef TILES_RESOURCES_HPP
#define TILES_RESOURCES_HPP

#include "Hades/resource_base.hpp"
#include "Hades/simple_resources.hpp"

namespace tiles
{
	//registers the resources needed to use tilemaps and the tile editor
	void RegisterTileResources(hades::data::data_system*);

	//tile sizes are capped by the data type used for texture sizes
	using tile_size_t = hades::resources::texture::size_type;
	using traits_list = std::vector<hades::data::UniqueId>;

	//data needed to render and interact with a tile
	//TODO: investigate storing the actual tex pointer here?
	struct tile
	{
		hades::data::UniqueId texture = hades::data::UniqueId::Zero;
		tile_size_t left = 0, top = 0;
		traits_list traits;
	};

	bool operator==(const tile &lhs, const tile &rhs);
	bool operator!=(const tile &lhs, const tile &rhs);

	bool operator<(const tile &lhs, const tile &rhs);

	namespace resources
	{
		void parseTileSettings(hades::data::UniqueId, const YAML::Node&, hades::data::data_manager*);
		std::vector<tile> ParseTileSection(hades::data::UniqueId texture, tile_size_t tile_size, YAML::Node &tiles_node,
			hades::types::string resource_type, hades::types::string name, hades::data::UniqueId mod);
		
		const hades::types::string tile_settings_name = "tile-settings";
		extern std::vector<hades::data::UniqueId> Tilesets;

		struct tile_settings_t {};

		struct tile_settings : public::hades::resources::resource_type<tile_settings_t>
		{
			hades::resources::texture::size_type tile_size = 0;
			hades::data::UniqueId error_tileset = hades::data::UniqueId::Zero;
			hades::data::UniqueId empty_tileset = hades::data::UniqueId::Zero;
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
