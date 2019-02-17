#include "hades/terrain.hpp"

#include "hades/data.hpp"
#include "hades/parser.hpp"
#include "hades/tiles.hpp"

namespace hades::resources
{
	static void parse_terrain(unique_id, const data::parser_node&, data::data_manager&);
	static void parse_terrainset(unique_id, const data::parser_node&, data::data_manager&);
}

namespace hades
{
	namespace detail
	{
		static find_make_texture_f find_make_texture{};
	}

	namespace id
	{
		static unique_id terrain_settings = unique_id::zero;
	}

	template<typename Func>
	void apply_to_terrain(resources::terrain &t, Func func)
	{
		//tileset tiles
		std::invoke(func, t.tiles);

		for (auto &vector : t.terrain_transition_tiles)
			std::invoke(func, vector);
	}

	void register_terrain_resources(data::data_manager &d, detail::find_make_texture_f func)
	{
		detail::find_make_texture = func;

		//create the terrain settings
		//and empty terrain first,
		//so that tiles can use them as tilesets without knowing
		//that they're really terrains
		id::terrain_settings = d.get_uid(resources::get_tile_settings_name());
		auto terrain_settings = d.find_or_create<resources::terrain_settings>(id::terrain_settings, unique_id::zero);

		auto empty_id = d.get_uid(resources::get_empty_tileset_name());
		auto empty = d.find_or_create<resources::terrain>(empty_id, unique_id::zero);

		//fill all the terrain tile lists with a default constructed tile
		apply_to_terrain(*empty, [](std::vector<resources::tile>&v) {
			v.emplace_back();
		});

		terrain_settings->empty_terrain = empty;
		terrain_settings->empty_tileset = empty;

		//register tile resources
		register_tiles_resources(d);

		//replace the tileset and tile settings parsers
		using namespace std::string_view_literals;
		//register tile resourcesd
		d.register_resource_type(resources::get_tilesets_name(), resources::parse_terrain);
		d.register_resource_type("terrainset"sv, resources::parse_terrainset);
	}

	terrain_vertex_position get_size(const terrain_map &t)
	{
		const auto tile_size = get_size(t.tile_layer);
		return tile_size + terrain_vertex_position{1, 1};
	}
}

namespace hades::resources
{
	static void load_terrain(resource_type<tileset_t> &r, data::data_manager &d)
	{
		assert(dynamic_cast<terrain*>(&r));

		auto &terr = static_cast<terrain&>(r);

		apply_to_terrain(terr, [&d](std::vector<tile> &v){
			for (auto &t : v)
			{
				if (t.texture)
				{
					const auto res = reinterpret_cast<const resource_base*>(t.texture);
					auto texture = d.get_resource(res->id);
					if (!texture->loaded)
						texture->load(d);
				}
			}
		});

		terr.loaded = true;
	}

	terrain::terrain() : tileset{ load_terrain } {}

	static void load_terrainset(resource_type<terrainset_t> &r, data::data_manager &d)
	{
		assert(dynamic_cast<terrainset*>(&r));

		auto &terr = static_cast<terrainset&>(r);

		for (const auto t : terr.terrains)
			d.get<terrain>(t->id);

		terr.loaded = true;
	}

	terrainset::terrainset() : resource_type{ load_terrainset } {}

	static void load_terrain_settings(resource_type<tile_settings_t> &r, data::data_manager &d)
	{
		assert(dynamic_cast<terrain_settings*>(&r));

		auto &s = static_cast<terrain_settings&>(r);

		detail::load_tile_settings(r, d);

		//load terrains
		if (s.empty_terrain)
			d.get<terrain>(s.empty_terrain->id);

		for (const auto t : s.terrains)
			d.get<terrain>(t->id);

		for (const auto t : s.terrainsets)
			d.get<terrainset>(t->id);

		s.loaded = true;
	}

	terrain_settings::terrain_settings() : tile_settings{ load_terrain_settings } {}

	template<class U = std::vector<tile>, class W>
	U& get_transition(transition_tile_type type, W& t)
	{
		assert(type < all);
		assert(type < std::size(t.terrain_transition_tiles));
		return t.terrain_transition_tiles[type];
	}

	static void add_tiles_to_terrain(terrain &terrain, const vector_int start_pos, const hades::resources::texture *tex,
		const std::vector<transition_tile_type> &tiles, const int32 tiles_per_row, const tile_size_t tile_size)
	{
		auto count = tile_size_t{};
		for (auto &t : tiles)
		{
			if (t >= transition_end)
				continue; //TODO: log error

			const auto tile_pos = vector_t{
				(count % tiles_per_row) * tile_size + start_pos.x,
				(count / tiles_per_row) * tile_size + start_pos.y
			};

			++count;

			auto &transition_vector = get_transition(t, terrain);

			assert(tile_pos.x >= 0 && tile_pos.y >= 0);
			const auto tile = resources::tile{ tex, tile_pos.x, tile_pos.y, &terrain };
			transition_vector.emplace_back(tile);
			terrain.tiles.emplace_back(tile);
		}
	}

	static std::vector<transition_tile_type> parse_layout(const std::vector<string> &s)
	{
		//layouts
		//This is the layout used by warcraft 3 tilesets, a common layout
		// on tileset websites
		constexpr auto war3_layout = std::array{ all, bottom_right, bottom_left, bottom_left_right,
				top_right, top_right_bottom_right, top_right_bottom_left, top_right_bottom_left_right,
				top_left, top_left_bottom_right, top_left_bottom_left, top_left_bottom_left_right,
				top_left_right, top_left_right_bottom_right, top_left_right_bottom_left, all };


		std::vector<transition_tile_type> out;

		//default to the war 3 layout
		if (s.empty())
			return { std::begin(war3_layout), std::end(war3_layout) };

		using namespace std::string_view_literals;

		//named based on which tile corners are 'empty'
		constexpr auto transition_names = std::array{
			"none"sv,
			"top-right"sv,
			"bottom-right"sv,
			"top-right_bottom-right"sv,
			"bottom-left"sv,
			"top-right_bottom-left"sv,
			"bottom-left-right"sv,
			"top-right_bottom-left-right"sv,
			"top-left"sv, 
			"top-left-right"sv,
			"top-left_bottom-right"sv,
			"top-left-right_bottom-right"sv,
			"top-left_bottom-left"sv,
			"top-left-right_bottom-left"sv,
			"top-left_bottom-left-right"sv
		};

		//check against named layouts
		if (s.size() == 1u)
		{
			if (s[0] == "default"sv ||
				s[0] == "war3"sv)
				return { std::begin(war3_layout), std::end(war3_layout) };
		}

		out.reserve(s.size());

		for (const auto &str : s)
		{
			for (auto i = std::size_t{}; i < std::size(transition_names); ++i)
			{
				if (transition_names[i] == str)
				{
					out.emplace_back(static_cast<transition_tile_type>(i));
					continue;
				}
			}

			out.emplace_back(transition_tile_type::transition_end);
		}

		return out;
	}

	static void parse_terrain_group(const unique_id m, terrain &t, const data::parser_node &p, data::data_manager &d, const tile_size_t tile_size)
	{
		// p:
		//	texture:
		//	left:
		//	top:
		//	tiles-per-row:
		//	tile_count:
		//	layout: //either war3, a single unique_id, or a set of tile_count unique_ids

		using namespace std::string_view_literals;
		using namespace data::parse_tools;

		const auto layout_str = get_sequence<string>(p, "layout"sv, {});
		auto transitions = parse_layout(layout_str);

		const auto tile_count = get_scalar<int32>(p, "tile-count"sv, -1);
		const auto tiles_per_row = get_scalar<int32>(p, "tiles-per-row"sv, -1);

		const auto left = get_scalar<int32>(p, "left"sv, 0);
		const auto top = get_scalar<int32>(p, "top"sv, 0);

		const auto texture_id = get_unique(p, "texture"sv, unique_id::zero);
		const auto texture = std::invoke(hades::detail::find_make_texture, d, texture_id, m);

		//make transitions list match the length of tile_count
		//looping if needed
		if (tile_count != -1)
		{
			const auto count = unsigned_cast(tile_count);
			//copy the range untill it is longer than tile_count
			while (count > std::size(transitions))
				std::copy(std::begin(transitions), std::end(transitions), std::back_inserter(transitions));

			//trim the vector back to tile_count length
			assert(count < std::size(transitions));
			transitions.resize(count);
		}

		add_tiles_to_terrain(t, { left, top }, texture, transitions, tiles_per_row, tile_size);
	}

	static void parse_terrain(unique_id m, const data::parser_node &p, data::data_manager &d)
	{
		//a terrain is also a tileset
		////
		//tilesets:
		//	sand: <// tileset name, these must be unique
		//		tags: <// a list of trait tags that get added to the tiles in this tileset; default: []
		//		tiles: <//for drawing individual tiles, we let the tileset parser handle it
		//			- {
		//				texture: <// texture to draw the tiles from
		//				left: <// pixel start of tileset; default: 0
		//				top: <// pixel left of tileset; default: 0
		//				tiles_per_row: <// number of tiles per row
		//				tile_count: <// total amount of tiles in tileset; default: tiles_per_row
		//			}
		//		terrain:
		//			- {
		//				texture:
		//				left:
		//				top:
		//				tiles-per-row:
		//				tile_count:
		//				layout: //either war3, a single unique_id, or a set of tile_count unique_ids
		//			}

		const auto terrains_list = p.get_children();
		auto settings = d.get<resources::terrain_settings>(id::terrain_settings);
		assert(settings);

		for (const auto &terrain_node : terrains_list)
		{
			using namespace std::string_view_literals;
			const auto name_n = terrain_node->get_child("name"sv);
			if (!name_n)
				continue;

			const auto name = name_n->to_string();
			const auto id = d.get_uid(name);

			auto terrain = d.find_or_create<resources::terrain>(id, m);
			assert(terrain);
			//parse_tiles will fill in the tags and tiles
			resources::detail::parse_tiles(m, *terrain, *settings, *terrain_node, d);

			const auto terrain_n = terrain_node->get_child("terrain"sv);
			if (terrain_n)
			{
				const auto terrain_group = terrain_n->get_children();
				for (const auto &group : terrain_group)
					parse_terrain_group(m, *terrain, *group, d, settings->tile_size);
			}

			settings->terrains.emplace_back(terrain);
		}
	}

	static void parse_terrainset(unique_id mod, const data::parser_node&, data::data_manager&)
	{
		//terrainsets:
		//	name: [terrains, terrains, terrains]
	}
}
//
//namespace ortho_terrain
//{
//	using hades::data::data_manager;
//
//	namespace resources
//	{
//		template<typename Func>
//		void ApplyToTerrain(terrain &t, Func func);
//
//		void ParseTerrain(hades::unique_id, const YAML::Node&, data_manager*);
//		void ParseTerrainSet(hades::unique_id, const YAML::Node&, data_manager*);
//	}
//
//	void CreateEmptyTerrain(hades::data::data_system *data)
//	{
//		resources::EmptyTerrainId = data->get_uid(tiles::empty_tileset_name);
//		hades::data::FindOrCreate<resources::terrain>(resources::EmptyTerrainId, hades::unique_id::zero, data);
//	}
//
//	void RegisterOrthoTerrainResources(hades::data::data_system* data)
//	{
//		//we need the tile resources registered
//		tiles::RegisterTileResources(data);
//
//		//data->register_resource_type("terrain", resources::ParseTerrain);
//		//data->register_resource_type("terrainsets", resources::ParseTerrainSet);
//
//		auto empty_t_tex = data->get<hades::resources::texture>(tiles::id::empty_tile_texture, hades::data::no_load);
//		auto empty_terrain = hades::data::FindOrCreate<resources::terrain>(resources::EmptyTerrainId, hades::unique_id::zero, data);
//		const tiles::tile empty_tile{ empty_t_tex, 0, 0 };
//		ApplyToTerrain(*empty_terrain, [empty_tile](auto &&vec) {
//			vec.emplace_back(empty_tile);
//		});
//	}
//
//	namespace resources
//	{
//		using namespace hades;
//
//		std::vector<hades::unique_id> TerrainSets;
//		hades::unique_id EmptyTerrainId = hades::unique_id::zero;
//
//		using tile_pos_t = hades::types::int32;
//
//		constexpr auto set_width = 4;
//		constexpr auto set_height = 4;
//		constexpr auto set_count = set_width * set_height;
//
//		void AddToTerrain(terrain &terrain, std::tuple<tile_pos_t, tile_pos_t> start_pos, const hades::resources::texture *tex,
//			std::array<transition2::TransitionTypes, set_count> tiles, tiles::tile_count_t tile_count = set_count)
//		{
//			const auto settings = tiles::GetTileSettings();
//			const auto tile_size = settings->tile_size;
//
//			constexpr auto columns = set_width;
//			constexpr auto rows = set_height;
//
//			const auto[x, y] = start_pos;
//
//			tile_size_t count = 0;
//			for (auto &t : tiles)
//			{
//				if (t <= transition2::TransitionTypes::NONE ||
//					t >= transition2::TransitionTypes::TRANSITION_END)
//					continue;
//
//				const auto [left, top] = tiles::GetGridPosition(count++, columns, tile_size);
//
//				auto &transition_vector = GetTransition(t, terrain);
//
//				const auto x_pos = left + x;
//				const auto y_pos = top + y;
//
//				assert(x_pos >= 0 && y_pos >= 0);
//				const tiles::tile tile{ tex, static_cast<tile_size_t>(x_pos), static_cast<tile_size_t>(y_pos) };
//				transition_vector.push_back(tile);
//				terrain.tiles.push_back(tile);
//
//				//we've loaded the requested number of tiles
//				if (count > tile_count)
//					break;
//			}
//		}
//
//		template<typename Func>
//		void ApplyToTerrain(terrain &t, Func func)
//		{
//			//tileset tiles
//			func(t.tiles);
//			//terrain tiles
//			func(t.full);
//			func(t.top_left_corner);
//			func(t.top_right_corner);
//			func(t.bottom_left_corner);
//			func(t.bottom_right_corner);
//			func(t.top);
//			func(t.left);
//			func(t.right);
//			func(t.bottom);
//			func(t.top_left_circle);
//			func(t.top_right_circle);
//			func(t.bottom_left_circle);
//			func(t.bottom_right_circle);
//			func(t.left_diagonal);
//			func(t.right_diagonal);
//		}
//
//		void LoadTerrain(hades::resources::resource_base *r, hades::data::data_manager *data)
//		{
//			auto t = static_cast<terrain*>(r);
//
//			ApplyToTerrain(*t, [data](auto &&arr) {
//				for (auto t : arr)
//				{
//					if(t.texture)
//						data->get<hades::resources::texture>(t.texture->id);
//				}
//			});
//		}
//
//		terrain::terrain() : tiles::resources::tileset(LoadTerrain) {}
//
//		void ParseTerrain(hades::unique_id mod, const YAML::Node& node, data_manager *data)
//		{
//			//a terrain is composed out of multiple terrain tilesets
//
//			//terrains:
//			//     -
//			//		  terrain: terrainname
//			//        source: textureid
//			//		  position: [x ,y]
//			//        type: one of {tile a specific tile id or name},
//			//				normal(a full set of transition tiles in the war3 layout)
//			//        traits: [] default is null
//			//        count: default is max = set_count }
//
//			using namespace transition2;
//
//			//normal warcraft 3 layout
//			constexpr std::array<transition2::TransitionTypes, set_count> normal{ ALL, BOTTOM_RIGHT, BOTTOM_LEFT, BOTTOM_LEFT_RIGHT,
//				TOP_RIGHT, TOP_RIGHT_BOTTOM_RIGHT, TOP_RIGHT_BOTTOM_LEFT, TOP_RIGHT_BOTTOM_LEFT_RIGHT,
//				TOP_LEFT, TOP_LEFT_BOTTOM_RIGHT, TOP_LEFT_BOTTOM_LEFT, TOP_LEFT_BOTTOM_LEFT_RIGHT,
//				TOP_LEFT_RIGHT, TOP_LEFT_RIGHT_BOTTOM_RIGHT, TOP_LEFT_RIGHT_BOTTOM_LEFT, ALL };
//
//			constexpr auto resource_type = "terrain";
//			constexpr auto position = "position";
//
//			std::vector<hades::unique_id> tilesets;
//
//			for (const auto &terrain : node)
//			{
//				//get the terrains name
//				const auto name = yaml_get_scalar<types::string>(terrain, resource_type, "n/a", "name", types::string{});
//				if (name.empty())
//					continue;
//
//				//get the correct terrain object
//				const auto terrain_id = data->get_uid(name);
//				auto t = hades::data::FindOrCreate<resources::terrain>(terrain_id, mod, data);
//				if (!t)
//				{
//					try
//					{
//						const auto tileset = data->get<tiles::resources::tileset>(terrain_id, hades::data::no_load);
//						LOGERROR("Terrains can be used as a tileset, but they must be defined as a terrain before being written to as a tileset");
//					}
//					catch(const hades::data::resource_wrong_type&)
//					{ /* we already posted an error for wrong type */ }
//
//					//no terrain to write to.
//					continue;
//				}
//
//				//texture source
//				const auto source = yaml_get_uid(terrain, resource_type, name, "source");
//
//				if (source == hades::unique_id::zero)
//					continue;
//
//				const auto texture = hades::data::get<hades::resources::texture>(source);
//
//				//get the start position of the tileset 
//				const auto pos_node = terrain[position];
//				if (pos_node.IsNull() || !pos_node.IsDefined())
//				{
//					LOGERROR("Terrain resource missing position element: expected 'position: [x, y]'");
//					continue;
//				}
//
//				if (!pos_node.IsSequence() || pos_node.size() != 2)
//				{
//					LOGERROR("Terrain resource position element in wrong format: expected 'position: [x, y]'");
//					continue;
//				}
//
//				const auto x = pos_node[0].as<tile_pos_t>();
//				const auto y = pos_node[1].as<tile_pos_t>();
//
//				const auto traits_str = yaml_get_sequence<types::string>(terrain, resource_type, name, "traits", mod);
//
//				const auto count = yaml_get_scalar<tiles::tile_count_t>(terrain, resource_type, name, "count", set_count); 
//
//				//type
//				const auto type = terrain["type"];
//				if (type.IsNull() || !type.IsDefined() || !type.IsScalar())
//				{
//					LOGERROR("Type missing for terrain: " + name);
//					continue;
//				}
//
//				if (const auto type_str = type.as<hades::types::string>(); !type_str.empty())
//				{
//					if (type_str == "normal")
//					{
//						AddToTerrain(*t, { x, y }, texture, normal, count);
//					}
//					else
//					{
//						LOGERROR("specified terrain tile layout by string, but didn't match one of the standard layouts string was: " + type_str);
//						continue;
//					}
//				}
//				else if (const auto type_int = type.as<tiles::tile_count_t>(); type_int > TRANSITION_BEGIN && type_int < TRANSITION_END)
//				{
//					auto make_array = [](tiles::tile_count_t type) {
//						std::array<transition2::TransitionTypes, set_count> arr;
//						assert(type > transition2::TransitionTypes::TRANSITION_BEGIN
//							&& type < transition2::TransitionTypes::TRANSITION_END);
//						arr.fill(static_cast<transition2::TransitionTypes>(type));
//						return arr;
//					};
//
//					const auto tiles = make_array(type_int);
//					AddToTerrain(*t, { x, y }, texture, tiles, count);
//				}
//				else
//				{
//					LOGERROR("Type is in wrong format: expected str or int for terrain: " + name);
//					continue;
//				}
//
//				//update traits for terrain tiles
//				//we got this far so the terrain data must have been valid and tiles must have been added to the terrain
//				//convert traits to uids and add them to the terrain
//				std::transform(std::begin(traits_str), std::end(traits_str), std::back_inserter(t->traits), [data](auto &&str) {
//					return data->get_uid(str);
//				});
//				
//				//remove any duplicates
//				remove_duplicates(t->traits);
//				const auto &traits_list = t->traits;
//				ApplyToTerrain(*t, [&traits_list](auto &&t_vec) {
//					for (auto &t : t_vec)
//						t.traits = traits_list;
//				});
//
//				tilesets.emplace_back(terrain_id);
//			}
//
//			//copy tilesets into the global Tileset list
//			//then remove any duplicates
//			auto &tiles_tilesets = tiles::resources::Tilesets;
//			std::copy(std::begin(tilesets), std::end(tilesets), std::back_inserter(tiles_tilesets));
//			remove_duplicates(tiles_tilesets);
//		}
//
//		void ParseTerrainSet(hades::unique_id mod, const YAML::Node& node, data_manager *data)
//		{
//			//terrainsets:
//			//     name: [terrain1, terrain2, ...]
//
//			constexpr auto resource_type = "terrainset";
//			
//			for (const auto &tset : node)
//			{
//				const auto name = tset.first.as<types::string>();
//				const auto terrainset_id = data->get_uid(name);
//
//				auto terrain_set = hades::data::FindOrCreate<terrainset>(terrainset_id, mod, data);
//
//				const auto terrain_list = tset.second;
//
//				if (!terrain_list.IsSequence())
//				{
//					LOGERROR("terrainset parse error, expected a squence of terrains");
//					continue;
//				}
//
//				std::vector<const terrain*> terrainset_order;
//
//				for (const auto terrain : terrain_list)
//				{
//					const auto terrain_name = terrain.as<hades::types::string>();
//					const auto terrain_id = data->get_uid(terrain_name);
//					if (terrain_id == hades::unique_id::zero)
//						continue;
//
//					const auto terrain_ptr = hades::data::FindOrCreate<resources::terrain>(terrain_id, mod, data);
//					if (!terrain_ptr)
//					{
//						LOGERROR("Unable to access terrain: " + terrain_name + ", mentioned as part of terrainset: " + name);
//						continue;
//					}
//
//					terrainset_order.emplace_back(terrain_ptr);
//				}
//
//				terrain_set->terrains.swap(terrainset_order);
//
//				TerrainSets.emplace_back(terrain_set->id);
//				remove_duplicates(TerrainSets);
//			}
//		}
//	}
//}
