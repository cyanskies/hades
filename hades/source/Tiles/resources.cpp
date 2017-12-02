#include "Tiles/resources.hpp"

#include "yaml-cpp/yaml.h"

#include "Tiles/tiles.hpp"

namespace tiles
{
	void RegisterTileResources(hades::data::data_manager* data)
	{
		data->register_resource_type("tile-settings", tiles::resources::parseTileSettings);
		data->register_resource_type("tilesets", tiles::resources::parseTileset);
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

	namespace resources
	{
		std::vector<hades::data::UniqueId> Tilesets;

		void parseTileSettings(hades::data::UniqueId mod, YAML::Node& node, hades::data::data_manager* data_manager)
		{
			//terrain-settings:
			//    tile-size: 32
			static const hades::types::string resource_type = tile_settings_name;
			static const hades::types::uint8 default_size = 8;
			auto id = data_manager->getUid(resource_type);
			tile_settings *settings = nullptr;
			if (!data_manager->exists(id))
			{
				//resource doens't exist yet, create it
				auto settings_ptr = std::make_unique<tile_settings>();
				settings = &*settings_ptr;
				data_manager->set<tile_settings>(id, std::move(settings_ptr));

				settings->tile_size = default_size;
				settings->id = id;
			}
			else
			{
				//retrieve it from the data store
				try
				{
					settings = data_manager->get<tile_settings>(id);
				}
				catch (hades::data::resource_wrong_type&)
				{
					//name is already used for something else, this cannnot be loaded
					auto mod_ptr = data_manager->getMod(mod);
					LOGERROR("Name collision with identifier: tile-settings, for tile-settings while parsing mod: " + mod_ptr->name + ". Name has already been used for a different resource type.");
					//skip the rest of this loop and check the next node
					return;
				}
			}

			settings->mod = mod;

			auto size = node["tile-size"];
			if (size.IsDefined() && yaml_error(resource_type, "n/a", "tile-size", "scalar", mod, size.IsScalar()))
				settings->tile_size = size.as<tile_size_t>(default_size);
		}

		void parseTiles(tileset& tset, hades::data::UniqueId texture, tile_size_t tile_size, tile_size_t top, tile_size_t left, tile_count_t width, tile_count_t count, const traits_list &traits)
		{
			tile_count_t row = 1, column = 1;
			while (count > 0)
			{
				tile t;
				t.texture = texture;
				t.traits = traits;
				t.left = column * left;
				t.top = row * top;

				if (column < width)
					column++;
				else
					row++;

				tset.tiles.push_back(t);
			}
		}

		void parseTileset(hades::data::UniqueId mod, YAML::Node& node, hades::data::data_manager *data)
		{
			//tilesets:
			//    sand: <// tileset name, these must be unique
			//        texture: <// texture to draw the tiles from
			//        V// if any of the following are provided then the remainder must also be provided //V
			//        left: <// pixel start of tileset
			//        top: <// pixel left of tileset
			//        tiles_per_row: <// number of tiles per row
			//        tile_count: <// total amount of tiles in tileset;
			//        traits: <// a list of trait tags that get added to the tiles in this tileset

			static const hades::types::string resource_type = "tileset";

			if (!yaml_error(resource_type, "n/a", "n/a", "map", mod, node.IsMap()))
				return;

			for (auto &n : node)
			{
				auto namenode = n.first;
				auto name = namenode.as<hades::types::string>();
				auto id = data->getUid(name);

				tileset* tset = nullptr;

				if (!data->exists(id))
				{
					auto tileset_ptr = std::make_unique<tileset>();
					tset = &*tileset_ptr;
					data->set<tileset>(id, std::move(tileset_ptr));

					tset->id = id;
					Tilesets.push_back(id);
				}
				else
				{
					try
					{
						tset = data->get<tileset>(id);
					}
					catch (hades::data::resource_wrong_type&)
					{
						//name is already used for something else, this cannnot be loaded
						auto modname = data->as_string(mod);
						LOGERROR("Failed to get tileset with id: " + name + ", in mod: " + modname + ", name has already been used for a different resource type.");
						//skip the rest of this loop and check the next node
						continue;
					}
				}

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

				tile_size_t left = yaml_get_scalar<tile_size_t>(node, resource_type, name, "left", mod, 0),
					top = yaml_get_scalar<tile_size_t>(node, resource_type, name, "top", mod, 0);
				tile_count_t width = yaml_get_scalar<tile_count_t>(node, resource_type, name, "tiles_per_row", mod, 0),
					count = yaml_get_scalar<tile_count_t>(node, resource_type, name, "tile_count", mod, 0);

				auto traits_str = yaml_get_sequence<hades::types::string>(node, resource_type, name, "traits", mod);
				traits_list traits;

				std::transform(std::begin(traits_str), std::end(traits_str), std::back_inserter(traits), [](hades::types::string s) {
					return hades::data_manager->getUid(s);
				});

				//get tile size
				auto tile_settings_id = data->getUid(tile_settings_name);
				auto tile_settings = data->get<resources::tile_settings>(tile_settings_id);

				parseTiles(*tset, texid, tile_settings->tile_size, top, left, width, count, traits);
			}
		}
	}
}