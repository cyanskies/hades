#include "Tiles/resources.hpp"

#include "yaml-cpp/yaml.h"

#include "Hades/Data.hpp"
#include "Hades/data_system.hpp"

#include "Tiles/editor.hpp"
#include "Tiles/tiles.hpp"

namespace tiles
{
	namespace resources
	{
		void parseTileset(hades::data::UniqueId, const YAML::Node&, hades::data::data_manager*);
	}

	void RegisterTileResources(hades::data::data_system* data)
	{	
		data->register_resource_type("tile-settings", tiles::resources::parseTileSettings);
		data->register_resource_type("tilesets", tiles::resources::parseTileset);

		//create error tileset and add a default error tile

		//create default tile settings obj
	}

	bool operator==(const tile &lhs, const tile &rhs)
	{
		return lhs.left == rhs.left &&
			lhs.texture == rhs.texture &&
			lhs.top == rhs.top;
	}

	bool operator!=(const tile &lhs, const tile &rhs)
	{
		return !(lhs == rhs);
	}

	bool operator<(const tile &lhs, const tile &rhs)
	{
		if (lhs.texture != rhs.texture)
			return lhs.texture < rhs.texture;
		else if(lhs.left != rhs.left)
			return lhs.left < rhs.left;
		else
			return lhs.top < rhs.top;
	}


	namespace resources
	{
		std::vector<hades::data::UniqueId> Tilesets;

		void parseTileSettings(hades::data::UniqueId mod, const YAML::Node& node, hades::data::data_manager* data_manager)
		{
			//terrain-settings:
			//    tile-size: 32
			static const hades::types::string resource_type = tile_settings_name;
			static const hades::types::uint8 default_size = 8;
			auto id = data_manager->getUid(resource_type);

			auto settings = hades::data::FindOrCreate<tile_settings>(id, mod, data_manager);

			if (!settings)
				return;

			settings->tile_size = yaml_get_scalar(node, resource_type, "n/a", "tile-size", mod, settings->tile_size);
			settings->error_tileset = yaml_get_uid(node, resource_type, "n/a", "error-tileset", mod, settings->error_tileset);
		}

		std::vector<tile> parseTiles(hades::data::UniqueId texture, tile_size_t tile_size, tile_size_t top, tile_size_t left, tile_count_t width, tile_count_t count, const traits_list &traits)
		{
			std::vector<tile> out;
			tile_count_t row = 0, column = 0;
			while (count > 0)
			{
				tile t;
				t.texture = texture;
				t.traits = traits;
				t.left = column * tile_size + left;
				t.top = row * tile_size + top;

				if (column < width)
					++column;
				else
				{
					column = 0;
					++row;
				}

				out.push_back(t);
				--count;
			}

			return out;
		}

		std::vector<tile> ParseTileSection(hades::data::UniqueId texture, tile_size_t tile_size, YAML::Node &tiles_node,
			hades::types::string resource_type, hades::types::string name, hades::data::UniqueId mod)
		{
			tile_size_t left = yaml_get_scalar<tile_size_t>(tiles_node, resource_type, name, "left", mod, 0),
				top = yaml_get_scalar<tile_size_t>(tiles_node, resource_type, name, "top", mod, 0);
			tile_count_t width = yaml_get_scalar<tile_count_t>(tiles_node, resource_type, name, "tiles_per_row", mod, 0),
				count = yaml_get_scalar<tile_count_t>(tiles_node, resource_type, name, "tile_count", mod, 0);

			auto traits_str = yaml_get_sequence<hades::types::string>(tiles_node, resource_type, name, "traits", mod);
			traits_list traits;

			std::transform(std::begin(traits_str), std::end(traits_str), std::back_inserter(traits), [](hades::types::string s) {
				return hades::data::GetUid(s);
			});

			return parseTiles(texture, tile_size, top, left, width - 1 /* make width 0 based */, count, traits);
		}

		void parseTileset(hades::data::UniqueId mod, const YAML::Node& node, hades::data::data_manager *data)
		{
			//tilesets:
			//    sand: <// tileset name, these must be unique
			//        texture: <// texture to draw the tiles from
			//        tiles: <// a section of tiles
			//            left: <// pixel start of tileset
			//            top: <// pixel left of tileset
			//            tiles_per_row: <// number of tiles per row
			//            tile_count: <// total amount of tiles in tileset;
			//            traits: <// a list of trait tags that get added to the tiles in this tileset

			static const hades::types::string resource_type = "tileset";

			if (!yaml_error(resource_type, "n/a", "n/a", "map", mod, node.IsMap()))
				return;

			for (auto n : node)
			{
				auto namenode = n.first;
				auto name = namenode.as<hades::types::string>();
				auto id = data->getUid(name);

				auto tset = hades::data::FindOrCreate<tileset>(id, mod, data);

				if (!tset)
					continue;

				tset->mod = mod;

				auto v = n.second;
				auto tex = v["texture"];
				if (!tex.IsDefined())
				{
					//tile set must have a texture
					LOGERROR("Tileset: " + name + ", doesn't define a texture, skipping");
					continue;
				}

				auto texid = data->getUid(tex.as<hades::types::string>());

				try
				{
					auto tile_settings = GetTileSettings();

					auto tiles_section = v["tiles"];
					if (!tiles_section.IsDefined())
					{
						LOGERROR("Cannot parse tiles section, skipping");
						continue;
					}

					auto tile_list = ParseTileSection(texid, tile_settings->tile_size, tiles_section, resource_type, name, mod);
					std::move(std::begin(tile_list), std::end(tile_list), std::back_inserter(tset->tiles));
				}
				catch (tile_map_exception&)
				{
					LOGERROR("Cannot load tileset: " + name + ", missing tile-settings resource.");
					continue;
				}
			}
		}
	}
}