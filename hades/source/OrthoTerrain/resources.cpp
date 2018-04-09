#include "OrthoTerrain/resources.hpp"

#include "yaml-cpp/yaml.h"

#include "Hades/Data.hpp"
#include "Hades/data_system.hpp"
#include "Hades/Logging.hpp"
#include "Hades/simple_resources.hpp"
#include "Hades/Types.hpp"

#include "Tiles/editor.hpp"
#include "Tiles/tiles.hpp"

namespace ortho_terrain
{
	namespace resources
	{
		void ParseTerrain(hades::data::UniqueId, const YAML::Node&, hades::data::data_manager*);
	}

	void RegisterOrthoTerrainResources(hades::data::data_system* data)
	{
		//we need the tile resources registered
		tiles::RegisterTileResources(data);

		data->register_resource_type("terrain", resources::ParseTerrain);
		//override the default layout and the tilesets parser
		//data->register_resource_type("terrainsets", ortho_terrain::resources::parseTileset);
		//create empty terrain
		resources::EmptyTerrainId = hades::UniqueId{};
	}

	namespace resources
	{
		using namespace hades;

		std::vector<hades::data::UniqueId> Terrains;
		hades::UniqueId EmptyTerrainId = hades::UniqueId::Zero;

		void ParseTerrain(hades::data::UniqueId mod, const YAML::Node& node, hades::data::data_manager *data)
		{
			//terrains:
			//		-
			//		  name:
			//        source: textureid
			//		  position: [x ,y]
			//        type: one of {tile a specific tile id or name},
			//				normal(a full set of transition tiles in the war3 layout)
			//        traits: []

			using namespace transition2;

			static const std::vector<tile_size_t> ascending =
			{ TOP_RIGHT, BOTTOM_RIGHT, TOP_RIGHT_BOTTOM_RIGHT,
				BOTTOM_LEFT, TOP_RIGHT_BOTTOM_LEFT, BOTTOM_LEFT_RIGHT,
				TOP_RIGHT_BOTTOM_LEFT_RIGHT, TOP_LEFT, TOP_LEFT_RIGHT,
				TOP_LEFT_BOTTOM_RIGHT, TOP_LEFT_RIGHT_BOTTOM_RIGHT, TOP_LEFT_BOTTOM_LEFT,
				TOP_LEFT_RIGHT_BOTTOM_LEFT, TOP_LEFT_BOTTOM_LEFT_RIGHT },
				brigids_cross =
			{ BOTTOM_LEFT, TOP_RIGHT_BOTTOM_RIGHT, TOP_LEFT_BOTTOM_LEFT_RIGHT, BOTTOM_LEFT_RIGHT,
				TOP_LEFT_BOTTOM_RIGHT, TOP_RIGHT_BOTTOM_LEFT_RIGHT, TOP_LEFT_BOTTOM_RIGHT, TOP_LEFT_RIGHT_BOTTOM_LEFT,
				TOP_RIGHT, TOP_LEFT_RIGHT, TOP_LEFT_RIGHT_BOTTOM_RIGHT, TOP_LEFT_BOTTOM_LEFT,
				TOP_RIGHT_BOTTOM_LEFT, BOTTOM_RIGHT, TOP_RIGHT_BOTTOM_LEFT, TOP_LEFT };
		}

		std::vector<tiles::tile> parseLayout(terrain_transition* transitions, data::UniqueId texture,
			data::UniqueId terrain1, data::UniqueId terrain2, std::vector<tile_size_t> tile_order,
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
					//info.terrain2 = terrain2;
				}

				//TODO: exception handling here
				auto &transition_vector = GetTransition(info.type, *transitions, data);
				transition_vector.push_back(ntile);

				out.push_back(ntile);
			}

			return out;
		}
	}
}
