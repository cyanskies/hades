#include "OrthoTerrain/resources.hpp"

#include "yaml-cpp/yaml.h"

#include "Hades/DataManager.hpp"
#include "Hades/data_manager.hpp"
#include "Hades/Logging.hpp"
#include "Hades/simple_resources.hpp"
#include "Hades/Types.hpp"

#include "Tiles/editor.hpp"
#include "Tiles/tiles.hpp"

namespace ortho_terrain
{
	namespace resources
	{
		//overrides the tileset parser provieded by tiles::, this one support loading terrain transitions as well.
		void parseTileset(hades::data::UniqueId mod, const YAML::Node& node, hades::data::data_manager*);
		void parseTerrainSettings(hades::data::UniqueId mod, const YAML::Node& node, hades::data::data_manager*);
	}

	void makeDefaultLayout(hades::data::data_manager* data)
	{
		auto layout_id = data->getUid(tiles::editor::tile_editor_layout);

		hades::resources::string *layout = hades::data::FindOrCreate<hades::resources::string>(layout_id, hades::data::UniqueId::Zero, data);

		if (!layout)
			return;

		layout->value =
			R"(ChildWindow {
				Position = (20, 20);
				Size = ("&.w / 6", "&.h / 1.5");
				Visible = true;
				Resizable = true;
				Title = "ToolBox";
				TitleButtons = None;

				SimpleVerticalLayout {
					Label {
						Text = "Terrain";
					}

					Label {
						Text = "Size:";
					}

					EditBox."terrain-size" {
						Size = ("&.w - 15", 25);
					}

					ScrollablePanel {
						Size = ("95%", "25%");
						Panel."terrain-selector" {
						}
					}

					Label {
						Text = "Tiles";
					}

					Label {
						Text = "Size:";
					}

					EditBox."draw-size" {
						Size = ("&.w - 15", 25);
					}

					ScrollablePanel {
						Size = ("95%", "25%");
						Panel."tile-selector" {
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

	void RegisterOrthoTerrainResources(hades::data::data_manager* data)
	{
		data->register_resource_type("terrain-settings", ortho_terrain::resources::parseTerrainSettings);

		//we need the tile resources registered
		tiles::RegisterTileResources(data);

		//override the default layout and the tilesets parser
		makeDefaultLayout(data);
		data->register_resource_type("tilesets", ortho_terrain::resources::parseTileset);
	}

	terrain_lookup TerrainLookup;

	namespace resources
	{
		using namespace hades;

		std::vector<hades::data::UniqueId> Terrains;
		std::vector<hades::data::UniqueId> Transitions;

		void parseTerrainSettings(hades::data::UniqueId mod, const YAML::Node& node, hades::data::data_manager* data_manager)
		{
			//terrain-settings:
			//    tile-size: 32
			const types::string resource_type = terrain_settings_name;
			const types::uint8 default_size = 8;
			auto id = data_manager->getUid(resource_type);
			terrain_settings *settings = hades::data::FindOrCreate<terrain_settings>(id, mod, data_manager);

			if (!settings)
				return;

			tiles::resources::parseTileSettings(mod, node, data_manager);

			settings->error_terrain = yaml_get_uid(node, resource_type, "terrain-settings", "error-terrain", mod, settings->error_terrain);
		}

		terrain *parseTerrain(hades::data::UniqueId mod, hades::data::UniqueId texture, YAML::Node& terrainNode, hades::data::data_manager *data)
		{
			//terrain: name <// a map of terrains in this tileset

			auto terrain_str = terrainNode.as<hades::types::string>();
			
			auto id = data->getUid(terrain_str);

			terrain* t = hades::data::FindOrCreate<terrain>(id, mod, data);
			if (t)
				Terrains.push_back(id);
			
			return t;
		}

		std::vector<tiles::tile> parseLayout(terrain_transition* transitions, data::UniqueId texture,
			data::UniqueId terrain1, data::UniqueId terrain2, std::vector<tile_size_t> tile_order, 
			tile_size_t left, tile_size_t top, tile_size_t columns, const tiles::traits_list &traits)
		{
			std::vector<tiles::tile> out;

			auto settings = tiles::GetTileSettings();
			auto tile_size = settings.tile_size;

			tile_size_t count = 0;
			for (auto &t : tile_order)
			{
				tiles::tile ntile;
				ntile.texture = texture;

				std::tie(ntile.left, ntile.top) = tiles::GetGridPosition(count++, columns, tile_size);

				//ofset by the position given
				ntile.left += left;
				ntile.top += top;

				std::copy(std::begin(traits), std::end(traits), std::back_inserter(ntile.traits));

				terrain_info info;
				info.type = static_cast<transition2::TransitionTypes>(t);

				if (info.type == transition2::NONE)
					info.terrain = terrain1;
				else if (info.type == transition2::ALL)
				{
					info.type = transition2::NONE;
					info.terrain = terrain2;
				}
				else
				{
					info.terrain = terrain1;
					info.terrain2 = terrain2;
				}
				
				auto &transition_vector = GetTransition(info.type, *transitions);
				transition_vector.push_back(ntile);

				out.push_back(ntile);
				TerrainLookup.insert({ ntile, info });
			}

			return out;
		}

		static const auto transition2_resource_type = "transition2";

		std::vector<tiles::tile> parseTransition2(hades::data::UniqueId mod, hades::data::UniqueId texture, YAML::Node& transitions, hades::data::data_manager* data)
		{
			//transition2:
					//terrain1 : sand
					//terrain2 : dirt
					//layout: ascending
					//left:
					//top:
					//width: <// how many tiles in each coloumn of the layout;
					//traits:

			auto v = transitions;
			const auto name = "n/a";
			hades::data::UniqueId id;

			auto terrain1_id = yaml_get_uid(v, transition2_resource_type, name, "terrain1", mod);
			auto terrain2_id = yaml_get_uid(v, transition2_resource_type, name, "terrain2", mod);

			terrain_transition* t = nullptr;

			if (!Transitions.empty())
			{
				//search transition list for a matching candidate
				auto either = [](data::UniqueId first, data::UniqueId second, terrain_transition* t)->bool
				{
					if ((first == t->terrain1 &&
						second == t->terrain2) ||
						(first == t->terrain2 &&
							second == t->terrain1))
						return true;
					else
						return false;
				};

				for (auto &t_id : Transitions)
				{
					auto *trans = data->get<terrain_transition>(t_id);

					if (either(terrain1_id, terrain2_id, trans))
						t = trans;
				}
			}

			//create if no matching transition exists
			if (!t)
			{
				t = data::FindOrCreate<terrain_transition>(id, mod, data);
				//if we fail to create then skip
				if (t == nullptr)
					return std::vector<tiles::tile>();

				t->terrain1 = terrain1_id;
				t->terrain2 = terrain2_id;
				Transitions.push_back(t->id);
			}

			terrain1_id = t->terrain1;
			terrain2_id = t->terrain2;
			t->mod = mod;

			auto layout = v["layout"];

			std::vector<tile_size_t> layout_order;
				
			using namespace transition2;

			static const std::vector<tile_size_t> ascending = 
			{ TOP_RIGHT, BOTTOM_RIGHT, TOP_RIGHT_BOTTOM_RIGHT,
				BOTTOM_LEFT, TOP_RIGHT_BOTTOM_LEFT, BOTTOM_LEFT_RIGHT,
				TOP_RIGHT_BOTTOM_LEFT_RIGHT, TOP_LEFT, TOP_LEFT_RIGHT, 
				TOP_LEFT_BOTTOM_RIGHT, TOP_LEFT_RIGHT_BOTTOM_RIGHT, TOP_LEFT_BOTTOM_LEFT,
				TOP_LEFT_RIGHT_BOTTOM_LEFT, TOP_LEFT_BOTTOM_LEFT_RIGHT},
				brigids_cross = 
			{ BOTTOM_LEFT, TOP_RIGHT_BOTTOM_RIGHT, TOP_LEFT_BOTTOM_LEFT_RIGHT, BOTTOM_LEFT_RIGHT,
				TOP_LEFT_BOTTOM_RIGHT, TOP_RIGHT_BOTTOM_LEFT_RIGHT, TOP_LEFT_BOTTOM_RIGHT, TOP_LEFT_RIGHT_BOTTOM_LEFT,
				TOP_RIGHT, TOP_LEFT_RIGHT, TOP_LEFT_RIGHT_BOTTOM_RIGHT, TOP_LEFT_BOTTOM_LEFT,
				TOP_RIGHT_BOTTOM_LEFT, BOTTOM_RIGHT, TOP_RIGHT_BOTTOM_LEFT, TOP_LEFT};

			if (layout.IsDefined() && layout.IsSequence())
			{
				for (auto &i : layout)
					layout_order.push_back(i.as<tile_size_t>());
			}
			else if (layout.IsDefined() && layout.IsScalar() &&
				layout.as<types::string>() == "brigid")
			{
				layout_order = brigids_cross;
			}
			else
			{
				//default layout
				layout_order = ascending;
			}

			tile_size_t toppos = yaml_get_scalar<tile_size_t>(v, transition2_resource_type, name, "top", mod, 0);
			tile_size_t leftpos = yaml_get_scalar<tile_size_t>(v, transition2_resource_type, name, "left", mod, 0);
			tile_size_t tiles_per_row = yaml_get_scalar<tile_size_t>(v, transition2_resource_type, name, "width", mod, 0);

			auto traits_str = yaml_get_sequence<hades::types::string>(v, transition2_resource_type, name, "traits", mod);
			tiles::traits_list traits;

			std::transform(std::begin(traits_str), std::end(traits_str), std::back_inserter(traits), [](hades::types::string s) {
				return hades::data_manager->getUid(s);
			});

			auto tiles = parseLayout(t, texture, terrain1_id, terrain2_id, layout_order, leftpos, toppos, tiles_per_row, traits);

			//add the transition to each of the terrains
			//TODO: this should be done when creating the transition, not when a new sheet is added too it.
			// check transition3 for this error as well.
			if (data_manager->exists(terrain1_id))
				data_manager->get<terrain>(terrain1_id)->transitions.push_back(t->id);

			if (data_manager->exists(terrain2_id))
				data_manager->get<terrain>(terrain2_id)->transitions.push_back(t->id);

			return tiles;
		}

		std::vector<tiles::tile> parseLayout(terrain_transition3* transitions, data::UniqueId texture,
			data::UniqueId terrain1, data::UniqueId terrain2, data::UniqueId terrain3, std::vector<tile_size_t> tile_order,
			tile_size_t left, tile_size_t top, tile_size_t columns, const tiles::traits_list &traits)
		{
			std::vector<tiles::tile> out;

			auto settings = tiles::GetTileSettings();
			auto tile_size = settings.tile_size;

			tile_size_t count = 0;
			for (auto &t : tile_order)
			{
				tiles::tile ntile;
				ntile.texture = texture;

				std::tie(ntile.left, ntile.top) = tiles::GetGridPosition(count++, columns, tile_size);

				//ofset by the position given
				ntile.left += left;
				ntile.top += top;

				std::copy(std::begin(traits), std::end(traits), std::back_inserter(ntile.traits));

				terrain_info info;
				info.type3 = static_cast<transition3::Types>(t);

				if (info.type3 == transition3::T1_NONE)
				{
					info.type = transition2::NONE;
					info.terrain = terrain1;
				}
				else if (info.type == transition3::T2_NONE)
				{
					info.type = transition2::NONE;
					info.terrain = terrain2;
				}
				else if (info.type == transition3::T3_NONE)
				{
					info.type = transition2::NONE;
					info.terrain = terrain3;
				}
				else
				{
					info.terrain = terrain1;
					info.terrain2 = terrain2;
					info.terrain3 = terrain3;
				}

				auto &transition_vector = GetTransition(info.type3, *transitions);
				transition_vector.push_back(ntile);

				out.push_back(ntile);
				TerrainLookup.insert({ ntile, info });
			}

			return out;
		}

		static const auto transition3_resource_type = "transition3";

		std::vector<tiles::tile> parseTransition3(hades::data::UniqueId mod, hades::data::UniqueId texture, YAML::Node& transitions, hades::data::data_manager* data)
		{
			//transition3:
				//terrain1 : sand
				//terrain2 : dirt
				//terrain3 : grass
				//layout: ascending
				//left:
				//top:
				//width: <// how many tiles in each coloumn of the layout;
				//traits:

			auto name = "n/a";
			auto v = transitions;

			auto id = data->getUid(name);

			auto terrain1_id = yaml_get_uid(v, transition3_resource_type, name, "terrain1", mod);
			auto terrain2_id = yaml_get_uid(v, transition3_resource_type, name, "terrain2", mod);
			auto terrain3_id = yaml_get_uid(v, transition3_resource_type, name, "terrain3", mod);

			terrain_transition3* t3 = nullptr;

			static std::vector<hades::data::UniqueId> Transitions;

			if (!Transitions.empty())
			{
				//FIXME: this doesnt seem finished
				std::vector<std::array<data::UniqueId, 3>> combinations = {
					{ terrain1_id, terrain2_id, terrain3_id },
					{ terrain1_id, terrain2_id, terrain3_id },
					{ terrain1_id, terrain2_id, terrain3_id },
					{ terrain1_id, terrain2_id, terrain3_id },
					{ terrain1_id, terrain2_id, terrain3_id },
					{ terrain1_id, terrain2_id, terrain3_id },
					{ terrain1_id, terrain2_id, terrain3_id },
					{ terrain1_id, terrain2_id, terrain3_id },
					{ terrain1_id, terrain2_id, terrain3_id },
				};

				auto either3 = [&combinations](terrain_transition3* t)->bool const
				{
					return false;
				};

				for (auto &t_id : Transitions)
				{
					auto *trans = data->get<terrain_transition3>(t_id);

					if (either3(trans))
						t3 = trans;
				}
			}

			//create if no matching transition exists
			if (t3 == nullptr)
			{
				t3 = data::FindOrCreate<terrain_transition3>(id, mod, data);
				t3->terrain1 = terrain1_id;
				t3->terrain2 = terrain2_id;
				t3->terrain3 = terrain3_id;

				//attach transition 2 references
				auto terrain1 = hades::data_manager->get<terrain>(terrain1_id),
					terrain2 = hades::data_manager->get<terrain>(terrain2_id);

				for (auto &t : terrain1->transitions)
				{
					auto transition = hades::data_manager->get<terrain_transition>(t);
					if (transition->terrain1 == terrain1_id &&
						transition->terrain2 == terrain2_id)
					{
						t3->transition_1_2 = t;
					}
					else if(transition->terrain1 == terrain1_id &&
						transition->terrain2 == terrain3_id)
					{
						t3->transition_1_3 = t;
					}
				}

				for (auto &t : terrain2->transitions)
				{
					auto transition = hades::data_manager->get<terrain_transition>(t);
					if (transition->terrain1 == terrain2_id &&
						transition->terrain2 == terrain3_id)
					{
						t3->transition_2_3 = t;
						break;
					}
				}

				Transitions.push_back(t3->id);
			}

			//if we fail to create then skip
			if (t3 == nullptr)
				return std::vector<tiles::tile>();

			terrain1_id = t3->terrain1;
			terrain2_id = t3->terrain2;
			terrain3_id = t3->terrain3;
			t3->mod = mod;

			bool custom_layout = false;
			auto layout = v["layout"];

			std::vector<tile_size_t> layout_order;

			//these values corrispond to entries in transition3::Types
			static const std::vector<tile_size_t> ascending = 
				{  5,  7, 11, 14, 15, 16,
				17, 19, 21, 22, 23, 25,
				29, 32, 33, 34, 35, 38,
				42, 45, 46, 47, 48, 51,
				55, 57, 58, 59, 61, 63, 
				64, 65, 66, 69, 73, 75 };

			if (layout.IsDefined() && layout.IsSequence())
			{
				for (auto &i : layout)
					layout_order.push_back(i.as<tile_size_t>());
			}
			else
			{
				//default layout
				layout_order = ascending;
			}

			tile_size_t toppos = yaml_get_scalar<tile_size_t>(v, transition3_resource_type, name, "top", mod, 0);
			tile_size_t leftpos = yaml_get_scalar<tile_size_t>(v, transition3_resource_type, name, "left", mod, 0);
			tile_size_t tiles_per_row = yaml_get_scalar<tile_size_t>(v, transition3_resource_type, name, "width", mod, 0);

			auto traits_str = yaml_get_sequence<hades::types::string>(v, transition2_resource_type, name, "traits", mod);
			tiles::traits_list traits;

			std::transform(std::begin(traits_str), std::end(traits_str), std::back_inserter(traits), [](hades::types::string s) {
				return hades::data_manager->getUid(s);
			});

			auto tiles = parseLayout(t3, texture, terrain1_id, terrain2_id, terrain3_id, layout_order, leftpos, toppos, tiles_per_row, traits);

			//add the transition to each of the terrains
			//TODO: this should be done when creating the transition, not when a new sheet is added too it.
			// check transition2 for this error as well.
			if (data_manager->exists(terrain1_id))
				data_manager->get<terrain>(terrain1_id)->transitions3.push_back(t3->id);

			if (data_manager->exists(terrain2_id))
				data_manager->get<terrain>(terrain2_id)->transitions3.push_back(t3->id);

			if (data_manager->exists(terrain3_id))
				data_manager->get<terrain>(terrain3_id)->transitions3.push_back(t3->id);

			return tiles;
		}

		void parseTileset(hades::data::UniqueId mod, const YAML::Node& node, hades::data::data_manager *data)
		{

			//==old 'tiles' tilesets(still compatible)
			//tilesets:
			//    sand: <// tileset name, these must be unique
			//        texture: <// texture to draw the tiles from
			//        tiles: <// a section of tiles
			//            left: <// pixel start of tileset
			//            top: <// pixel left of tileset
			//            tiles_per_row: <// number of tiles per row
			//            tile_count: <// total amount of tiles in tileset;
			//            traits: <// a list of trait tags that get added to the tiles in this tileset

			//tilesets:
				//sand: <// tileset name, these must be unique
					//texture: <// the texture to load this terrain from
					//terrain-traits: [trait1, trait2] <// traits applied to any tiles with this terrain
					//terrain: sand <// all the terrain-traits will be associated with this terrain
									//  also the tiles: entry will be used for this terrain
					//transition2:
			            //terrain1: sand
						//terrain2: snow
						//left: // left pixel pos of the tile grid
						//top: //top pixel pos of the tile grid
						//width:
						//layout:
			            //traits:
					//transition3:
						//terrain1: sand
						//terrain2: snow
						//terrain3: grass
						//left: // left pixel pos of the tile grid
						//top: //top pixel pos of the tile grid
						//width:
						//layout:
			            //traits:
			static const types::string resource_type = "tileset";

			if (!yaml_error(resource_type, "n/a", "n/a", "map", mod, node.IsMap()))
				return;

			terrain *ter = nullptr;

			for (auto &n : node)
			{
				auto namenode = n.first;
				auto name = namenode.as<types::string>();
				auto id = data->getUid(name);

				using tileset = tiles::resources::tileset;

				tileset* tset = hades::data::FindOrCreate<tileset>(id, mod, data);

				if (!tset)
					continue;

				auto v = n.second;
				auto tex = v["texture"];
				if (!tex.IsDefined())
				{
					//tile set must have a texture
					LOGERROR("Tileset: " + name + ", doesn't define a texture, skipping");
					continue;
				}

				auto texid = data->getUid(tex.as<types::string>());

				//for all the entries in the tileset
				//should have one terrain and one transition entry
				auto terrain = v["terrain"];
				if(terrain.IsDefined())
					ter = parseTerrain(mod, texid, terrain, data);

				auto traits = yaml_get_sequence<hades::types::string>(v, resource_type, name, "terrain-traits", mod);

				std::transform(std::begin(traits), std::end(traits), std::back_inserter(ter->traits),
					[](hades::types::string s) {
					return hades::data_manager->getUid(s);
				});

				auto tiles_section = v["tiles"];
				if (tiles_section.IsDefined())
				{
					auto settings = tiles::GetTileSettings();
					auto tiles = tiles::resources::ParseTileSection(texid, settings.tile_size, tiles_section, resource_type, name, mod);
					std::copy(std::begin(tiles), std::end(tiles), std::back_inserter(tset->tiles));
					if (ter)
					{
						std::copy(std::begin(tiles), std::end(tiles), std::back_inserter(ter->tiles));

						//insert info for these tiles into the terrain lookup table
						for (const auto &t : tiles)
						{
							terrain_info i;
							i.terrain = ter->id;
							i.type = transition2::NONE;

							TerrainLookup.insert({ t, i });
						}
					}
				}

				auto transition = v[transition2_resource_type];
				if(transition.IsDefined())
				{
					auto tiles = parseTransition2(mod, texid, transition, data);
					tset->tiles.insert(tset->tiles.end(), tiles.begin(), tiles.end());
				}

				auto transition3 = v[transition3_resource_type];
				if (transition3.IsDefined())
				{
					auto tiles = parseTransition3(mod, texid, transition3, data);
					tset->tiles.insert(tset->tiles.end(), tiles.begin(), tiles.end());
				}
			}
		}
	}
}
