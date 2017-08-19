#include "OrthoTerrain/resources.hpp"

#include "yaml-cpp/yaml.h"

#include "Hades/Logging.hpp"
#include "Hades/simple_resources.hpp"
#include "Hades/Types.hpp"

namespace ortho_terrain
{
	void RegisterOrthoTerrainResources(hades::data::data_manager* data)
	{
		data->register_resource_type("terrain-settings", ortho_terrain::resources::parseTerrainSettings);
		data->register_resource_type("tilesets", ortho_terrain::resources::parseTileset);
	}

	bool operator==(const tile &lhs, const tile &rhs)
	{
		return lhs.left == rhs.left &&
			lhs.terrain == rhs.terrain &&
			lhs.texture == rhs.texture &&
			lhs.top == rhs.top;
	}

	bool operator!=(const tile &lhs, const tile &rhs)
	{
		return !(lhs == rhs);
	}

	namespace resources
	{
		using namespace hades;

		std::vector<hades::data::UniqueId> Terrains;
		std::vector<hades::data::UniqueId> Tilesets;
		std::vector<hades::data::UniqueId> Transitions;

		std::tuple<hades::types::int32, hades::types::int32> GetGridPosition(hades::types::uint32 tile_number,
			hades::types::uint32 tiles_per_row, tile_size_t tile_size)
		{
			return std::make_tuple((tile_number % tiles_per_row) * tile_size,
				static_cast<int>(std::floor(tile_number / static_cast<float>(tiles_per_row)) * tile_size));
		}

		template<class T>
		T* FindOrCreate(data::UniqueId target, data::UniqueId mod, data::data_manager* data)
		{
			T* r = nullptr;

			if (!data->exists(target))
			{
				auto new_ptr = std::make_unique<T>();
				r = &*new_ptr;
				data->set<T>(target, std::move(new_ptr));

				r->id = target;
			}
			else
			{
				try
				{
					r = data->get<T>(target);
				}
				catch (data::resource_wrong_type&)
				{
					//name is already used for something else, this cannnot be loaded
					auto modname = data->as_string(mod);
					LOGERROR("Failed to get " + std::string(typeid(T).name()) + " with id: " + data->as_string(target) + ", in mod: " + modname + ", name has already been used for a different resource type.");
				}
			}

			return r;
		}

		void parseTerrainSettings(hades::data::UniqueId mod, YAML::Node& node, hades::data::data_manager* data_manager)
		{
			//terrain-settings:
			//    tile-size: 32
			const types::string resource_type = terrain_settings_name;
			const types::uint8 default_size = 8;
			auto id = data_manager->getUid(resource_type);
			terrain_settings *settings = nullptr;
			if (!data_manager->exists(id))
			{
				//resource doens't exist yet, create it
				auto settings_ptr = std::make_unique<terrain_settings>();
				settings = &*settings_ptr;
				data_manager->set<terrain_settings>(id, std::move(settings_ptr));

				settings->tile_size = default_size;
				settings->id = id;
			}
			else
			{
				//retrieve it from the data store
				try
				{
					settings = data_manager->get<terrain_settings>(id);
				}
				catch (data::resource_wrong_type&)
				{
					//name is already used for something else, this cannnot be loaded
					auto mod_ptr = data_manager->getMod(mod);
					LOGERROR("Name collision with identifier: terrain-settings, for texture while parsing mod: " + mod_ptr->name + ". Name has already been used for a different resource type.");
					//skip the rest of this loop and check the next node
					return;
				}
			}

			settings->mod = mod;

			auto size = node["tile-size"];
			if (size.IsDefined() && yaml_error(resource_type, "n/a", "tile-size", "scalar", mod, size.IsScalar()))
				settings->tile_size = size.as<tile_size_t>(default_size);

			auto error_id = node["error-tile"];
			if (error_id.IsDefined() && yaml_error(resource_type, "n/a", "error-tile", "scalar", mod, error_id.IsScalar()))
				settings->error_tile = hades::data_manager->getUid(error_id.as<types::string>(hades::data::no_id_string));
		}

		std::vector<tile> parseTerrain(hades::data::UniqueId mod, hades::data::UniqueId texture, YAML::Node& terrainNode, hades::data::data_manager *data)
		{
			//terrain: <// a map of terrains in this tileset
				//sand: <//terrains can be added in multiple tilesets, they will overrigtthemselves
					//left: 15 <// the location of the tile in the tilset
					//top: 15 <// ""
				//sand: <// creating a new entry with the same name, alternative tile for this terrain
					//left: 15
					// top : 32

			const types::string resource_type = "terrain";

			std::vector<tile> out;

			if (!yaml_error(resource_type, "n/a", "n/a", "map", mod, terrainNode.IsMap()))
				return out;

			for (auto &n : terrainNode)
			{
				auto namenode = n.first;
				auto name = namenode.as<types::string>();
				auto id = data->getUid(name);

				terrain* t = nullptr;

				if (!data->exists(id))
				{
					auto terrain_ptr = std::make_unique<terrain>();
					t = &*terrain_ptr;
					data->set<terrain>(id, std::move(terrain_ptr));

					t->id = id;

					Terrains.push_back(id);
				}
				else
				{
					try
					{
						t = data->get<terrain>(id);
					}
					catch (data::resource_wrong_type&)
					{
						//name is already used for something else, this cannnot be loaded
						auto modname = data->as_string(mod);
						LOGERROR("Failed to get terrain with id: " + name + ", in mod: " + modname + ", name has already been used for a different resource type.");
						//skip the rest of this loop and check the next node
						continue;
					}
				}

				t->mod = mod;
				tile ntile;

				ntile.terrain = id;
				ntile.texture = texture;
				ntile.type = transition2::TransitionTypes::NONE;

				auto v = n.second;
				auto left = v["left"];
				if (left.IsDefined() && yaml_error(resource_type, name, "left", "scalar", mod, left.IsScalar()))
					ntile.left = left.as<tile_size_t>(0);

				auto top = v["top"];
				if (top.IsDefined() && yaml_error(resource_type, name, "top", "scalar", mod, top.IsScalar()))
					ntile.top = top.as<tile_size_t>(0);

				t->tiles.push_back(ntile);
				out.push_back(ntile);
			}

			return out;
		}

		std::vector<tile> parseLayout(terrain_transition* transitions, data::UniqueId texture, data::UniqueId terrain1, data::UniqueId terrain2, std::vector<tile_size_t> tile_order, tile_size_t left, tile_size_t top, tile_size_t columns)
		{
			std::vector<tile> out;

			auto setting_id = hades::data_manager->getUid(ortho_terrain::resources::terrain_settings_name);
			//if terrain settings was missing, we should have errored out earlier.
			//TODO: add better error handling here.
			assert(hades::data_manager->exists(setting_id));
			auto tile_size = hades::data_manager->get<terrain_settings>(setting_id)->tile_size;

			tile_size_t count = 0;
			for (auto &t : tile_order)
			{
				tile ntile;
				ntile.texture = texture;

				std::tie(ntile.left, ntile.top) = GetGridPosition(count++, columns, tile_size);

				//ofset by the position given
				ntile.left += left;
				ntile.top += top;
				ntile.type = static_cast<transition2::TransitionTypes>(t);

				if (ntile.type == transition2::NONE)
					ntile.terrain = terrain1;
				else if (ntile.type == transition2::ALL)
				{
					ntile.type = transition2::NONE;
					ntile.terrain = terrain2;
				}
				else
				{
					ntile.terrain = terrain1;
					ntile.terrain2 = terrain2;
				}
				
				auto &transition_vector = GetTransition(ntile.type, *transitions);
				transition_vector.push_back(ntile);

				out.push_back(ntile);
			}

			return out;
		}

		static const auto transition2_resource_type = "transition2";

		std::vector<tile> parseTransition2(hades::data::UniqueId mod, hades::data::UniqueId texture, YAML::Node& transitions, hades::data::data_manager* data)
		{
			//terrain - transitions:
				//#the name of the transition isn't important and can be duplicated or 
				//#changed, but any transition with the same terrain1 and terrain2 will be considered the same
				//#even if the order is swapped, the cannonical order is the one first supplied
				//sand - dirt:
					//terrain1 : sand
					//$TODO: support this
					//#if terrain2 is absent then the transition can be drawn over any terrain
					//#this can be used for terrain that sits on top of other terrains, like concrete
					//#such a transition is expected to have some transparent area to see the other terrain through,
					//#may just have a hard edge
					//terrain2 : dirt
					//#a transition is expected to take up 4 * tilesize by 4 * tilesize pixels
					//layout: ascending
					//#this just specifies. the top left corner of the transition grid
					//#any transition tile that is entirely alpha is assumed to be empty and
					//#ignored for autofill purposes, but still uses up an id
					//left:
					//top:
					//width: <// how many tiles in each coloumn of the layout; default: 4

			
			std::vector<tile> out;

			if (!yaml_error(transition2_resource_type, "n/a", "n/a", "map", mod, transitions.IsMap()))
				return out;

			for (auto &n : transitions)
			{
				auto namenode = n.first;
				auto name = namenode.as<types::string>();
				auto v = n.second;

				auto id = data->getUid(name);

				auto terrain1_id = data::UniqueId::Zero;
				auto terrain1 = v["terrain1"];
				if (terrain1.IsDefined() && yaml_error(transition2_resource_type, name, "terrain1", "scalar", mod, terrain1.IsScalar()))
					terrain1_id = data->getUid(terrain1.as<types::string>());

				auto terrain2_id = data::UniqueId::Zero;
				auto terrain2 = v["terrain2"];
				if (terrain2.IsDefined() && yaml_error(transition2_resource_type, name, "terrain2", "scalar", mod, terrain2.IsScalar()))
					terrain2_id = data->getUid(terrain2.as<types::string>());

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
				if (t == nullptr)
				{
					t = FindOrCreate<terrain_transition>(id, mod, data);
					t->terrain1 = terrain1_id;
					t->terrain2 = terrain2_id;
					Transitions.push_back(t->id);
				}

				//if we fail to create then skip
				if (t == nullptr)
					continue;

				terrain1_id = t->terrain1;
				terrain2_id = t->terrain2;
				t->mod = mod;

				bool custom_layout = false;
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

				tile_size_t toppos = 0;
				auto top = v["top"];
				if (top.IsDefined() && yaml_error(transition2_resource_type, name, "top", "scalar", mod, top.IsScalar()))
					toppos = top.as<tile_size_t>();

				tile_size_t leftpos = 0;
				auto left = v["left"];
				if (left.IsDefined() && yaml_error(transition2_resource_type, name, "left", "scalar", mod, left.IsScalar()))
					leftpos = left.as<tile_size_t>();

				tile_size_t tiles_per_row = 0;
				auto row_length = v["width"];
				if (row_length.IsDefined() && yaml_error(transition2_resource_type, name, "width", "scalar", mod, left.IsScalar()))
					tiles_per_row = row_length.as<tile_size_t>(4);

				auto tiles = parseLayout(t, texture, terrain1_id, terrain2_id, layout_order, leftpos, toppos, tiles_per_row);

				//add the transition to each of the terrains
				if (data_manager->exists(terrain1_id))
					data_manager->get<terrain>(terrain1_id)->transitions.push_back(t->id);

				if (data_manager->exists(terrain2_id))
					data_manager->get<terrain>(terrain2_id)->transitions.push_back(t->id);

				out.insert(out.end(), tiles.begin(), tiles.end());
			}

			return out;
		}

		std::vector<tile> parseLayout(terrain_transition3* transitions, data::UniqueId texture, data::UniqueId terrain1, data::UniqueId terrain2, data::UniqueId terrain3, std::vector<tile_size_t> tile_order, tile_size_t left, tile_size_t top, tile_size_t columns)
		{
			std::vector<tile> out;

			auto setting_id = hades::data_manager->getUid(ortho_terrain::resources::terrain_settings_name);
			assert(hades::data_manager->exists(setting_id));
			auto tile_size = hades::data_manager->get<terrain_settings>(setting_id)->tile_size;

			tile_size_t count = 0;
			for (auto &t : tile_order)
			{
				tile ntile;
				ntile.texture = texture;

				std::tie(ntile.left, ntile.top) = GetGridPosition(count++, columns, tile_size);

				//ofset by the position given
				ntile.left += left;
				ntile.top += top;
				ntile.type3 = static_cast<transition3::Types>(t);

				if (ntile.type3 == transition3::T1_NONE)
				{
					ntile.type = transition2::NONE;
					ntile.terrain = terrain1;
				}
				else if (ntile.type == transition3::T2_NONE)
				{
					ntile.type = transition2::NONE;
					ntile.terrain = terrain2;
				}
				else if (ntile.type == transition3::T3_NONE)
				{
					ntile.type = transition2::NONE;
					ntile.terrain = terrain3;
				}
				else
				{
					ntile.terrain = terrain1;
					ntile.terrain2 = terrain2;
					ntile.terrain3 = terrain3;
				}

				auto &transition_vector = GetTransition(ntile.type3, *transitions);
				transition_vector.push_back(ntile);

				out.push_back(ntile);
			}

			return out;
		}

		static const auto transition3_resource_type = "transition3";

		std::vector<tile> parseTransition3(hades::data::UniqueId mod, hades::data::UniqueId texture, YAML::Node& transitions, hades::data::data_manager* data)
		{
			//terrain - transitions:
				//#the name of the transition isn't important and can be duplicated or 
				//#changed, but any transition with the same terrain1 and terrain2 will be considered the same
				//#even if the order is swapped, the cannonical order is the one first supplied
				//sand - dirt - water:
					//terrain1 : sand
					//terrain2 : dirt
					//terrain3 : water
					//#a transition is expected to take up 6 * tilesize by 6 * tilesize pixels
					//layout: ascending
					//#this just specifies. the top left corner of the transition grid
					//#any transition tile that is entirely alpha is assumed to be empty and
					//#ignored for autofill purposes, but still uses up an id
					//left:
					//top:
					//width: <// how many tiles in each coloumn of the layout; default: 6

			std::vector<tile> out;

			if (!yaml_error(transition3_resource_type, "n/a", "n/a", "map", mod, transitions.IsMap()))
				return out;

			for (auto &n : transitions)
			{
				auto namenode = n.first;
				auto name = namenode.as<types::string>();
				auto v = n.second;

				auto id = data->getUid(name);

				auto terrain1_id = data::UniqueId::Zero;
				auto terrain1 = v["terrain1"];
				if (terrain1.IsDefined() && yaml_error(transition3_resource_type, name, "terrain1", "scalar", mod, terrain1.IsScalar()))
					terrain1_id = data->getUid(terrain1.as<types::string>());

				auto terrain2_id = data::UniqueId::Zero;
				auto terrain2 = v["terrain2"];
				if (terrain2.IsDefined() && yaml_error(transition3_resource_type, name, "terrain2", "scalar", mod, terrain2.IsScalar()))
					terrain2_id = data->getUid(terrain2.as<types::string>());

				auto terrain3_id = data::UniqueId::Zero;
				auto terrain3 = v["terrain3"];
				if (terrain3.IsDefined() && yaml_error(transition3_resource_type, name, "terrain2", "scalar", mod, terrain2.IsScalar()))
					terrain3_id = data->getUid(terrain3.as<types::string>());

				terrain_transition3* t3 = nullptr;

				static std::vector<hades::data::UniqueId> Transitions;

				if (!Transitions.empty())
				{
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
					t3 = FindOrCreate<terrain_transition3>(id, mod, data);
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
					continue;

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

				tile_size_t toppos = 0;
				auto top = v["top"];
				if (top.IsDefined() && yaml_error(transition3_resource_type, name, "top", "scalar", mod, top.IsScalar()))
					toppos = top.as<tile_size_t>(0);

				tile_size_t leftpos = 0;
				auto left = v["left"];
				if (left.IsDefined() && yaml_error(transition3_resource_type, name, "left", "scalar", mod, left.IsScalar()))
					leftpos = left.as<tile_size_t>(0);

				tile_size_t tiles_per_row = 6;
				auto row_length = v["width"];
				if (row_length.IsDefined() && yaml_error(transition3_resource_type, name, "width", "scalar", mod, left.IsScalar()))
					tiles_per_row = row_length.as<tile_size_t>(6);

				auto tiles = parseLayout(t3, texture, terrain1_id, terrain2_id, terrain3_id, layout_order, leftpos, toppos, tiles_per_row);

				//add the transition to each of the terrains
				//TODO: this should be done when creating the transition, not when a new sheet is added too it.
				// check transition2 for this error as well.
				if (data_manager->exists(terrain1_id))
					data_manager->get<terrain>(terrain1_id)->transitions3.push_back(t3->id);

				if (data_manager->exists(terrain2_id))
					data_manager->get<terrain>(terrain2_id)->transitions3.push_back(t3->id);

				if (data_manager->exists(terrain3_id))
					data_manager->get<terrain>(terrain3_id)->transitions3.push_back(t3->id);

				out.insert(out.end(), tiles.begin(), tiles.end());
			}

			return out;
		}

		void parseTileset(hades::data::UniqueId mod, YAML::Node& node, hades::data::data_manager *data)
		{
			//tilesets:
				//sand: <// tileset name, these must be unique
					//texture: <// the texture to load this terrain from
					//terrain: <// a map of terrains in this tileset
						//sand: <//terrains can be added in multiple tilesets, they will overrigtthemselves
							//left: 15 <// the location of the tile in the tilset
							//top: 15 <// ""
							//traits: <// list of terrain traits, used 
							//	-land
							//	-nobuild
						//sand: <// creating a new entry with the same name, alternative tile for this terrain
							//left: 15
							// top : 32

			static const types::string resource_type = "tileset";

			if (!yaml_error(resource_type, "n/a", "n/a", "map", mod, node.IsMap()))
				return;

			for (auto &n : node)
			{
				auto namenode = n.first;
				auto name = namenode.as<types::string>();
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
					catch (data::resource_wrong_type&)
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

				auto texid = data->getUid(tex.as<types::string>());

				//for all the entries in the tileset
				//should have one terrain and one transition entry
				auto terrain = v["terrain"];
				if(terrain.IsDefined())
				{
					auto tiles = parseTerrain(mod, texid, terrain, data);
					tset->tiles.insert(tset->tiles.end(), tiles.begin(), tiles.end());
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
