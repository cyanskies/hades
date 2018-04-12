#include "OrthoTerrain/resources.hpp"

#include "yaml-cpp/yaml.h"

#include "Hades/data_system.hpp"
#include "Hades/Logging.hpp"

#include "Tiles/tiles.hpp"

namespace ortho_terrain
{
	using hades::data::UniqueId;
	using hades::data::data_manager;

	namespace resources
	{
		void ParseTerrain(UniqueId, const YAML::Node&, data_manager*);
	}

	void RegisterOrthoTerrainResources(hades::data::data_system* data)
	{
		//we need the tile resources registered
		tiles::RegisterTileResources(data);

		data->register_resource_type("terrain", resources::ParseTerrain);
		//override the default layout and the tilesets parser
		//data->register_resource_type("terrainsets", ortho_terrain::resources::parseTileset);
		//create empty terrain
		resources::EmptyTerrainId = UniqueId{};
	}

	namespace resources
	{
		using namespace hades;

		std::vector<UniqueId> Terrains;
		UniqueId EmptyTerrainId = UniqueId::Zero;

		using tile_pos_t = hades::types::int32;

		void AddToTerrain(terrain &t, std::tuple<tile_pos_t, tile_pos_t>, const hades::resources::texture *tex, std::vector<transition2::TransitionTypes> tiles);

		void ApplyTraits(std::vector<tiles::tile> &tiles, tiles::traits_list traits)
		{
			for (auto &t : tiles)
			{
				t.traits.clear();
				std::swap(t.traits, traits);
			}
		}

		//applys the terrains traits to all of its member tiles
		void ApplyTraits(terrain &t)
		{
			ApplyTraits(t.tile, t.traits);
			ApplyTraits(t.top_left_corner, t.traits);
			ApplyTraits(t.top_right_corner, t.traits);
			ApplyTraits(t.bottom_left_corner, t.traits);
			ApplyTraits(t.bottom_right_corner, t.traits);
			ApplyTraits(t.top, t.traits);
			ApplyTraits(t.left, t.traits);
			ApplyTraits(t.right, t.traits);
			ApplyTraits(t.bottom, t.traits);
			ApplyTraits(t.top_left_circle, t.traits);
			ApplyTraits(t.top_right_circle, t.traits);
			ApplyTraits(t.bottom_left_circle, t.traits);
			ApplyTraits(t.bottom_right_circle, t.traits);
			ApplyTraits(t.left_diagonal, t.traits);
			ApplyTraits(t.right_diagonal, t.traits);
		}

		void ParseTerrain(UniqueId mod, const YAML::Node& node, data_manager *data)
		{
			//terrains:
			//		- {
			//		  name:
			//        source: textureid
			//		  position: [x ,y]
			//        type: one of {tile a specific tile id or name},
			//				normal(a full set of transition tiles in the war3 layout)
			//        traits: [] 
			//        count: default is max = set_count }

			constexpr auto set_width = 4;
			constexpr auto set_height = 4;
			constexpr auto set_count = set_width * set_height;

			using namespace transition2;

			//normal warcraft 3 layout with
			constexpr std::array<tile_size_t, set_count> normal{ NONE, BOTTOM_RIGHT, BOTTOM_LEFT, BOTTOM_LEFT_RIGHT,
				TOP_RIGHT, TOP_RIGHT_BOTTOM_RIGHT, TOP_RIGHT_BOTTOM_LEFT, TOP_RIGHT_BOTTOM_LEFT_RIGHT,
				TOP_LEFT, TOP_LEFT_BOTTOM_RIGHT, TOP_LEFT_BOTTOM_LEFT, TOP_LEFT_BOTTOM_LEFT_RIGHT,
				TOP_LEFT_RIGHT, TOP_LEFT_RIGHT_BOTTOM_RIGHT, TOP_LEFT_RIGHT_BOTTOM_LEFT, NONE };

			constexpr auto resource_type = "terrain";
			constexpr auto position = "position";

			for (const auto &terrain : node)
			{
				//get the terrains name
				const auto name = yaml_get_scalar<types::string>(terrain, resource_type, "n/a", "name", types::string{});
				if (name.empty())
					continue;

				//get the correct terrain object
				const auto terrain_id = data->getUid(name);
				auto t = hades::data::FindOrCreate<resources::terrain>(terrain_id, mod, data);

				//texture source
				const auto source = yaml_get_uid(terrain, resource_type, name, "source");

				if (source == UniqueId::Zero)
					continue;

				const auto texture = hades::data::Get<hades::resources::texture>(source);

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

				}
				else if (const auto type_int = type.as<tiles::tile_count_t>(); type_int > TRANSITION_BEGIN && type_int < TRANSITION_END)
				{

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
					return data->getUid(str);
				});
				
				//remove any duplicates
				std::sort(std::begin(t->traits), std::end(t->traits));
				std::unique(std::begin(t->traits), std::end(t->traits));
				ApplyTraits(*t);
			}
		}

		std::vector<tiles::tile> parseLayout(UniqueId texture,
			UniqueId terrain1, data::UniqueId terrain2, std::vector<tile_size_t> tile_order,
			tile_size_t left, tile_size_t top, tile_size_t columns, const tiles::traits_list &traits, hades::data::data_manager *data)
		{
			std::vector<tiles::tile> out;

			auto settings = tiles::GetTileSettings();
			auto tile_size = settings->tile_size;

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

				//terrain_info info;
				//info.type = static_cast<transition2::TransitionTypes>(t);

				//if (info.type == transition2::NONE)
				//	info.terrain = terrain1;
				//else if (info.type == transition2::ALL)
				//{
				//	info.type = transition2::NONE;
				//	info.terrain = terrain2;
				//}
				//else
				//{
				//	info.terrain = terrain1;
				//	//info.terrain2 = terrain2;
				//}

				//TODO: exception handling here
				//auto &transition_vector = GetTransition(info.type, *transitions, data);
				//transition_vector.push_back(ntile);

				out.push_back(ntile);
			}

			return out;
		}
	}
}
