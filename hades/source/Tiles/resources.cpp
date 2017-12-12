#include "Tiles/resources.hpp"

#include "yaml-cpp/yaml.h"

#include "Hades/DataManager.hpp"
#include "Hades/data_manager.hpp"

#include "Tiles/editor.hpp"
#include "Tiles/tiles.hpp"

namespace tiles
{
	void makeDefaultLayout(hades::data::data_manager* data)
	{
		auto layout_id = data->getUid(editor::tile_editor_layout);

		auto layout = hades::data::FindOrCreate<hades::resources::string>(layout_id, hades::data::UniqueId::Zero, data);
		layout->value =
			R"(ChildWindow {
				Position = (20, 20);
				Size = ("&.w / 6", "&.h / 1.5");
				Visible = true;
				Resizable = true;
				Title = "ToolBox";
				TitleButtons = None;

				ScrollablePanel {
					SimpleVerticalLayout {
						Label {
							Text = "Tiles";
						}

						Label {
							Text = "Size:";
						}

						EditBox."draw-size" {
							Size = ("&.w - 15", 25);
						}

						Panel."tile-selector" { 
							Size = ("&.w - 5", "&.h");
						}
					}
				}
			}

			ChildWindow."load_dialog" {
				Position = (200, 200);
				Title = "Load...";
				Label."load_mod_label" {
					Position = (5, 5);
					Text = "Mod:";
				}

				EditBox."load_mod" {
					Position = (5, "load_mod_label.y + load_mod_label.h + 5");
					Size = (100, 30);
					DefaultText = "./";
				}

				EditBox."load_filename" {
					Position = ("load_mod.x + load_mod.w + 10", "load_mod.y");
					Size = (100, 30);
					DefaultText = "new.lvl";
				}

				Label {
					Position = ("load_filename.x", 5);
					Text = "Filename:";
				}

				Button."load_button" {
					Position = ("load_filename.x + load_filename.w + 10" , "load_mod.y");
					Text = "Load";
				}

				Size = ("load_button.x + load_button.w + 5", "load_filename.y + load_filename.h + 5");
			}

			ChildWindow."new_dialog" {
				Position = (150, 150);
				Title = "New...";

				Label."new_mod_label" {
					Text = "Mod:";
					Position = (5, 5);
				}

				EditBox."new_mod" {
					Position = (5, "new_mod_label.x + new_mod_label.h + 10");
					Size = (100, 30);
					DefaultText = "./";
                
				}

				EditBox."new_filename" {
					Position = ("new_mod.x + new_mod.w + 10", "new_mod.y");
					Size = (100, 30);
					DefaultText = "new.lvl";
				}

				Label {
					Position = ("new_filename.x", "new_mod_label.y");
					Text = "Filename:";
				}

				Label."new_sizex_label" {
					Position = ("new_mod_label.x", "new_filename.y + new_filename.h + 10");
					Text = "Width:";
				}

				EditBox."new_sizex" {
					Position = ("new_sizex_label.x + new_sizex_label.w + 10", "new_sizex_label.y");
					Size = (50, 30);
					DefaultText = "25";
				}

				Label."new_sizey_label" {
					Position = ("new_sizex.x + new_sizex.w + 10", "new_sizex_label.y");
					Text = "Height:";
				}

				EditBox."new_sizey" {
					Position = ("new_sizey_label.x + new_sizey_label.w + 10", "new_sizey_label.y");
					Size = (50, 30);
					DefaultText = "25";
				}

				Label."new_gen_label" {
					Position = ("new_sizex_label.x", "new_sizey.y + new_sizey.h + 10");
					Text = "Generator:";
				}

				ComboBox."new_gen" {
					Position = ("new_gen_label.x + new_gen_label.w + 10", "new_gen_label.y");
				}

				Button."new_button" {
					Position = ("new_filename.x + new_filename.w + 10", "new_filename.y");
					Text = "Create";
				}

				Size = ("new_button.x + new_button.w + 5", "new_gen.y + new_gen.h + 5");
			})";
	}

	void RegisterTileResources(hades::data::data_manager* data)
	{
		makeDefaultLayout(data);
		
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

		void parseTileSettings(hades::data::UniqueId mod, YAML::Node& node, hades::data::data_manager* data_manager)
		{
			//terrain-settings:
			//    tile-size: 32
			static const hades::types::string resource_type = tile_settings_name;
			static const hades::types::uint8 default_size = 8;
			auto id = data_manager->getUid(resource_type);
			
			auto settings = hades::data::FindOrCreate<tile_settings>(id, mod, data_manager);

			if (!settings)
				return;

			settings->tile_size = yaml_get_scalar(node, resource_type, "n/a", "tile-size", mod, settings->tile_size);		}

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
				return hades::data_manager->getUid(s);
			});

			return parseTiles(texture, tile_size, top, left, width - 1 /* make width 0 based */, count, traits);
		}

		void parseTileset(hades::data::UniqueId mod, YAML::Node& node, hades::data::data_manager *data)
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

					auto tile_list = ParseTileSection(texid, tile_settings.tile_size, tiles_section, resource_type, name, mod);
					std::copy(std::begin(tile_list), std::end(tile_list), std::back_inserter(tset->tiles));
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