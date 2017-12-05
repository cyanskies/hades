#include "Tiles/resources.hpp"

#include "yaml-cpp/yaml.h"

#include "Tiles/editor.hpp"
#include "Tiles/tiles.hpp"

namespace tiles
{
	void makeDefaultLayout(hades::data::data_manager* data)
	{
		auto layout_id = data->getUid(editor::tile_editor_layout);

		hades::resources::string *layout = nullptr;

		if (!data->exists(layout_id))
		{
			//resource doens't exist yet, create it
			auto layout_ptr = std::make_unique<hades::resources::string>();
			layout = &*layout_ptr;
			data->set<hades::resources::string>(layout_id, std::move(layout_ptr));
		}
		else
		{
			layout = data->get<hades::resources::string>(layout_id);
		}

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

				tset.tiles.push_back(t);
				--count;
			}
		}

		void ParseTilesSection(tileset& tset, hades::data::UniqueId texture, tile_size_t tile_size, YAML::Node &tiles_node,
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

			parseTiles(tset, texture, tile_size, top, left, width - 1 /* make width 0 based */, count, traits);
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

				try
				{
					//get tile size
					auto tile_settings_id = data->getUid(tile_settings_name);
					auto tile_settings = data->get<resources::tile_settings>(tile_settings_id);

					auto tiles_sections = v["tiles"];
					if (!tiles_sections.IsDefined())
					{
						LOGERROR("Cannot parse tiles section, skipping");
						continue;
					}

					ParseTilesSection(*tset, texid, tile_settings->tile_size, tiles_sections, resource_type, name, mod);
				}
				catch (hades::data::resource_null&)
				{
					LOGERROR("Cannot load tileset: " + name + ", missing tile-settings resource.");
					continue;
				}
			}
		}
	}
}