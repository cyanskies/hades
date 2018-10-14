#include "OrthoTerrain/resources.hpp"

#include "yaml-cpp/yaml.h"

#include "Hades/data_system.hpp"
#include "Hades/Logging.hpp"
#include "hades/utility.hpp"

#include "Tiles/tiles.hpp"

namespace ortho_terrain
{
	using hades::data::data_manager;

	namespace resources
	{
		template<typename Func>
		void ApplyToTerrain(terrain &t, Func func);

		void ParseTerrain(hades::unique_id, const YAML::Node&, data_manager*);
		void ParseTerrainSet(hades::unique_id, const YAML::Node&, data_manager*);
	}

	void CreateEmptyTerrain(hades::data::data_system *data)
	{
		resources::EmptyTerrainId = data->get_uid(tiles::empty_tileset_name);
		hades::data::FindOrCreate<resources::terrain>(resources::EmptyTerrainId, hades::unique_id::zero, data);
	}

	void RegisterOrthoTerrainResources(hades::data::data_system* data)
	{
		//we need the tile resources registered
		tiles::RegisterTileResources(data);

		data->register_resource_type("terrain", resources::ParseTerrain);
		data->register_resource_type("terrainsets", resources::ParseTerrainSet);

		auto empty_t_tex = data->get<hades::resources::texture>(tiles::id::empty_tile_texture, hades::data::no_load);
		auto empty_terrain = hades::data::FindOrCreate<resources::terrain>(resources::EmptyTerrainId, hades::unique_id::zero, data);
		const tiles::tile empty_tile{ empty_t_tex, 0, 0 };
		ApplyToTerrain(*empty_terrain, [empty_tile](auto &&vec) {
			vec.emplace_back(empty_tile);
		});
	}

	namespace resources
	{
		using namespace hades;

		std::vector<hades::unique_id> TerrainSets;
		hades::unique_id EmptyTerrainId = hades::unique_id::zero;

		using tile_pos_t = hades::types::int32;

		constexpr auto set_width = 4;
		constexpr auto set_height = 4;
		constexpr auto set_count = set_width * set_height;

		void AddToTerrain(terrain &terrain, std::tuple<tile_pos_t, tile_pos_t> start_pos, const hades::resources::texture *tex,
			std::array<transition2::TransitionTypes, set_count> tiles, tiles::tile_count_t tile_count = set_count)
		{
			const auto settings = tiles::GetTileSettings();
			const auto tile_size = settings->tile_size;

			constexpr auto columns = set_width;
			constexpr auto rows = set_height;

			const auto[x, y] = start_pos;

			tile_size_t count = 0;
			for (auto &t : tiles)
			{
				if (t <= transition2::TransitionTypes::NONE ||
					t >= transition2::TransitionTypes::TRANSITION_END)
					continue;

				const auto [left, top] = tiles::GetGridPosition(count++, columns, tile_size);

				auto &transition_vector = GetTransition(t, terrain);

				const auto x_pos = left + x;
				const auto y_pos = top + y;

				assert(x_pos >= 0 && y_pos >= 0);
				const tiles::tile tile{ tex, static_cast<tile_size_t>(x_pos), static_cast<tile_size_t>(y_pos) };
				transition_vector.push_back(tile);
				terrain.tiles.push_back(tile);

				//we've loaded the requested number of tiles
				if (count > tile_count)
					break;
			}
		}

		template<typename Func>
		void ApplyToTerrain(terrain &t, Func func)
		{
			//tileset tiles
			func(t.tiles);
			//terrain tiles
			func(t.full);
			func(t.top_left_corner);
			func(t.top_right_corner);
			func(t.bottom_left_corner);
			func(t.bottom_right_corner);
			func(t.top);
			func(t.left);
			func(t.right);
			func(t.bottom);
			func(t.top_left_circle);
			func(t.top_right_circle);
			func(t.bottom_left_circle);
			func(t.bottom_right_circle);
			func(t.left_diagonal);
			func(t.right_diagonal);
		}

		void LoadTerrain(hades::resources::resource_base *r, hades::data::data_manager *data)
		{
			auto t = static_cast<terrain*>(r);

			ApplyToTerrain(*t, [data](auto &&arr) {
				for (auto t : arr)
				{
					if(t.texture)
						data->get<hades::resources::texture>(t.texture->id);
				}
			});
		}

		terrain::terrain() : tiles::resources::tileset(LoadTerrain) {}

		void ParseTerrain(hades::unique_id mod, const YAML::Node& node, data_manager *data)
		{
			//a terrain is composed out of multiple terrain tilesets

			//terrains:
			//     -
			//		  terrain: terrainname
			//        source: textureid
			//		  position: [x ,y]
			//        type: one of {tile a specific tile id or name},
			//				normal(a full set of transition tiles in the war3 layout)
			//        traits: [] default is null
			//        count: default is max = set_count }

			using namespace transition2;

			//normal warcraft 3 layout
			constexpr std::array<transition2::TransitionTypes, set_count> normal{ ALL, BOTTOM_RIGHT, BOTTOM_LEFT, BOTTOM_LEFT_RIGHT,
				TOP_RIGHT, TOP_RIGHT_BOTTOM_RIGHT, TOP_RIGHT_BOTTOM_LEFT, TOP_RIGHT_BOTTOM_LEFT_RIGHT,
				TOP_LEFT, TOP_LEFT_BOTTOM_RIGHT, TOP_LEFT_BOTTOM_LEFT, TOP_LEFT_BOTTOM_LEFT_RIGHT,
				TOP_LEFT_RIGHT, TOP_LEFT_RIGHT_BOTTOM_RIGHT, TOP_LEFT_RIGHT_BOTTOM_LEFT, ALL };

			constexpr auto resource_type = "terrain";
			constexpr auto position = "position";

			std::vector<hades::unique_id> tilesets;

			for (const auto &terrain : node)
			{
				//get the terrains name
				const auto name = yaml_get_scalar<types::string>(terrain, resource_type, "n/a", "name", types::string{});
				if (name.empty())
					continue;

				//get the correct terrain object
				const auto terrain_id = data->get_uid(name);
				auto t = hades::data::FindOrCreate<resources::terrain>(terrain_id, mod, data);
				if (!t)
				{
					try
					{
						const auto tileset = data->get<tiles::resources::tileset>(terrain_id, hades::data::no_load);
						LOGERROR("Terrains can be used as a tileset, but they must be defined as a terrain before being written to as a tileset");
					}
					catch(const hades::data::resource_wrong_type&)
					{ /* we already posted an error for wrong type */ }

					//no terrain to write to.
					continue;
				}

				//texture source
				const auto source = yaml_get_uid(terrain, resource_type, name, "source");

				if (source == hades::unique_id::zero)
					continue;

				const auto texture = hades::data::get<hades::resources::texture>(source);

				//get the start position of the tileset 
				const auto pos_node = terrain[position];
				if (pos_node.IsNull() || !pos_node.IsDefined())
				{
					LOGERROR("Terrain resource missing position element: expected 'position: [x, y]'");
					continue;
				}

				if (!pos_node.IsSequence() || pos_node.size() != 2)
				{
					LOGERROR("Terrain resource position element in wrong format: expected 'position: [x, y]'");
					continue;
				}

				const auto x = pos_node[0].as<tile_pos_t>();
				const auto y = pos_node[1].as<tile_pos_t>();

				const auto traits_str = yaml_get_sequence<types::string>(terrain, resource_type, name, "traits", mod);

				const auto count = yaml_get_scalar<tiles::tile_count_t>(terrain, resource_type, name, "count", set_count); 

				//type
				const auto type = terrain["type"];
				if (type.IsNull() || !type.IsDefined() || !type.IsScalar())
				{
					LOGERROR("Type missing for terrain: " + name);
					continue;
				}

				if (const auto type_str = type.as<hades::types::string>(); !type_str.empty())
				{
					if (type_str == "normal")
					{
						AddToTerrain(*t, { x, y }, texture, normal, count);
					}
					else
					{
						LOGERROR("specified terrain tile layout by string, but didn't match one of the standard layouts string was: " + type_str);
						continue;
					}
				}
				else if (const auto type_int = type.as<tiles::tile_count_t>(); type_int > TRANSITION_BEGIN && type_int < TRANSITION_END)
				{
					auto make_array = [](tiles::tile_count_t type) {
						std::array<transition2::TransitionTypes, set_count> arr;
						assert(type > transition2::TransitionTypes::TRANSITION_BEGIN
							&& type < transition2::TransitionTypes::TRANSITION_END);
						arr.fill(static_cast<transition2::TransitionTypes>(type));
						return arr;
					};

					const auto tiles = make_array(type_int);
					AddToTerrain(*t, { x, y }, texture, tiles, count);
				}
				else
				{
					LOGERROR("Type is in wrong format: expected str or int for terrain: " + name);
					continue;
				}

				//update traits for terrain tiles
				//we got this far so the terrain data must have been valid and tiles must have been added to the terrain
				//convert traits to uids and add them to the terrain
				std::transform(std::begin(traits_str), std::end(traits_str), std::back_inserter(t->traits), [data](auto &&str) {
					return data->get_uid(str);
				});
				
				//remove any duplicates
				remove_duplicates(t->traits);
				const auto &traits_list = t->traits;
				ApplyToTerrain(*t, [&traits_list](auto &&t_vec) {
					for (auto &t : t_vec)
						t.traits = traits_list;
				});

				tilesets.emplace_back(terrain_id);
			}

			//copy tilesets into the global Tileset list
			//then remove any duplicates
			auto &tiles_tilesets = tiles::resources::Tilesets;
			std::copy(std::begin(tilesets), std::end(tilesets), std::back_inserter(tiles_tilesets));
			remove_duplicates(tiles_tilesets);
		}

		void ParseTerrainSet(hades::unique_id mod, const YAML::Node& node, data_manager *data)
		{
			//terrainsets:
			//     name: [terrain1, terrain2, ...]

			constexpr auto resource_type = "terrainset";
			
			for (const auto &tset : node)
			{
				const auto name = tset.first.as<types::string>();
				const auto terrainset_id = data->get_uid(name);

				auto terrain_set = hades::data::FindOrCreate<terrainset>(terrainset_id, mod, data);

				const auto terrain_list = tset.second;

				if (!terrain_list.IsSequence())
				{
					LOGERROR("terrainset parse error, expected a squence of terrains");
					continue;
				}

				std::vector<const terrain*> terrainset_order;

				for (const auto terrain : terrain_list)
				{
					const auto terrain_name = terrain.as<hades::types::string>();
					const auto terrain_id = data->get_uid(terrain_name);
					if (terrain_id == hades::unique_id::zero)
						continue;

					const auto terrain_ptr = hades::data::FindOrCreate<resources::terrain>(terrain_id, mod, data);
					if (!terrain_ptr)
					{
						LOGERROR("Unable to access terrain: " + terrain_name + ", mentioned as part of terrainset: " + name);
						continue;
					}

					terrainset_order.emplace_back(terrain_ptr);
				}

				terrain_set->terrains.swap(terrainset_order);

				TerrainSets.emplace_back(terrain_set->id);
				remove_duplicates(TerrainSets);
			}
		}
	}
}
