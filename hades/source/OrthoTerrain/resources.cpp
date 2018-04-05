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
		//overrides the tileset parser provieded by tiles::, this one support loading terrain transitions as well.
		void parseTileset(hades::data::UniqueId mod, const YAML::Node& node, hades::data::data_manager*);
	}

	void RegisterOrthoTerrainResources(hades::data::data_system* data)
	{
		//we need the tile resources registered
		tiles::RegisterTileResources(data);

		//override the default layout and the tilesets parser
		data->register_resource_type("terrainsets", ortho_terrain::resources::parseTileset);
		//create empty terrain
		//create error terrain
	}

	namespace resources
	{
		using namespace hades;

		std::vector<hades::data::UniqueId> Terrains;

		terrain *parseTerrain(hades::data::UniqueId mod, hades::data::UniqueId texture, YAML::Node& terrainNode, hades::data::data_manager *data)
		{
			//terrains:
			//    name:
			//        source:
			//        type: tile, normal, [explicit]

			auto terrain_str = terrainNode.as<hades::types::string>();

			auto id = data->getUid(terrain_str);

			terrain* t = hades::data::FindOrCreate<terrain>(id, mod, data);

			return t;
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
					info.terrain2 = terrain2;
				}

				//TODO: exception handling here
				auto &transition_vector = GetTransition(info.type, *transitions, data);
				transition_vector.push_back(ntile);

				out.push_back(ntile);
			}

			return out;
		}

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
				for (const auto &i : layout)
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
				return hades::data::GetUid(s);
			});

			auto tiles = parseLayout(t, texture, terrain1_id, terrain2_id, layout_order, leftpos, toppos, tiles_per_row, traits, data);

			//add the transition to each of the terrains
			//TODO: this should be done when creating the transition, not when a new sheet is added too it.
			// check transition3 for this error as well.
			if (hades::data::Exists(terrain1_id))
				data->get<terrain>(terrain1_id)->transitions.push_back(t->id);

			if (data->exists(terrain2_id))
				data->get<terrain>(terrain2_id)->transitions.push_back(t->id);

			return tiles;
		}
	}
}
