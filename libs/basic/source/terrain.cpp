#include "hades/terrain.hpp"

#include <map>

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

	void register_terrain_resources(data::data_manager &d)
	{
		register_terrain_resources(d,
			[] (data::data_manager&, unique_id, unique_id)->resources::texture* {
			return nullptr; 
		});
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

		const auto empty_tile = resources::tile{ nullptr, 0u, 0u, empty };

		//fill all the terrain tile lists with a default constructed tile
		apply_to_terrain(*empty, [&empty_tile](std::vector<resources::tile>&v) {
			v.emplace_back(empty_tile);
		});

		terrain_settings->empty_terrain = empty;
		terrain_settings->empty_tileset = empty;

		//register tile resources
		register_tiles_resources(d, func);

		//replace the tileset and tile settings parsers
		using namespace std::string_view_literals;
		//register tile resources
		d.register_resource_type(resources::get_tilesets_name(), resources::parse_terrain);
		d.register_resource_type("terrainset"sv, resources::parse_terrainset);
	}

	static terrain_count_t get_terrain_index(const resources::terrainset *set, const resources::terrain *t)
	{
		assert(set && t);

		for (auto i = terrain_count_t{}; i < set->terrains.size(); ++i)
			if (set->terrains[i] == t)
				return i;

		throw terrain_error{"tried to index a terrain that isn't in this terrain set"};
	}

	terrain_map to_terrain_map(const raw_terrain_map &r)
	{
		auto m = terrain_map{};

		m.terrainset = data::get<resources::terrainset>(r.terrainset);
		
		const auto empty = resources::get_empty_terrain();
		std::transform(std::begin(r.terrain_vertex), std::end(r.terrain_vertex),
			std::back_inserter(m.terrain_vertex), [empty, &terrains = m.terrainset->terrains](terrain_count_t t) {
			if (t == terrain_count_t{})
				return empty;

			assert(t <= std::size(terrains));
			return terrains[t - 1u];
		});

		m.tile_layer = to_tile_map(r.tile_layer);
		std::transform(std::begin(r.terrain_layers), std::end(r.terrain_layers),
			std::back_inserter(m.terrain_layers), to_tile_map);

		return m;
	}

	raw_terrain_map to_raw_terrain_map(const terrain_map &t)
	{
		auto m = raw_terrain_map{};

		m.terrainset = t.terrainset->id;

		//build a replacement lookup table
		auto t_map = std::map<const resources::terrain*, terrain_count_t>{};
		const auto empty = resources::get_empty_terrain();
		//add the empty terrain with the index 0
		t_map.emplace(empty, terrain_count_t{});
		//add the rest of the terrains, offset the index by 1
		for (auto i = terrain_count_t{}; i < std::size(t.terrainset->terrains); ++i)
			t_map.emplace(t.terrainset->terrains[i], i + 1u);

		std::transform(std::begin(t.terrain_vertex), std::end(t.terrain_vertex),
			std::back_inserter(m.terrain_vertex), [t_map](const resources::terrain *t) {
			return t_map.at(t);
		});

		m.tile_layer = to_raw_map(t.tile_layer);

		std::transform(std::begin(t.terrain_layers), std::end(t.terrain_layers),
			std::back_inserter(m.terrain_layers), to_raw_map);

		return m;
	}

	terrain_map make_map(tile_position size, const resources::terrainset *terrainset, const resources::terrain *t)
	{
		assert(terrainset);
		assert(t);

		const auto index = get_terrain_index(terrainset, t);

		auto map = terrain_map{};
		map.terrainset = terrainset;
		map.terrain_vertex = std::vector<const resources::terrain*>((size.x + 1) * (size.y + 1), t);

		const auto empty_layer = make_map(size, resources::get_empty_tile());

		const auto end = std::size(terrainset->terrains);
		for (auto i = std::size_t{}; i < end; ++i)
		{
			if (i == index)
			{
				map.terrain_layers.emplace_back(make_map(
					size,
					resources::get_random_tile(*terrainset->terrains[i],
						resources::transition_tile_type::none)
				));
			}
			else 
				map.terrain_layers.emplace_back(empty_layer);
		}

		map.tile_layer = empty_layer;

		return map;
	}

	terrain_count_t get_width(const terrain_map &m)
	{
		return m.tile_layer.width + 1;
	}

	terrain_vertex_position get_size(const terrain_map &t)
	{
		const auto tile_size = get_size(t.tile_layer);
		return tile_size + terrain_vertex_position{1, 1};
	}

	bool within_map(terrain_vertex_position s, terrain_vertex_position p)
	{
		if (p.x < 0 ||
			p.y < 0 ||
			p.x > s.x ||
			p.y > s.y)
			return false;

		return true;
	}

	bool within_map(const terrain_map &m, terrain_vertex_position p)
	{
		const auto s = get_size(m);
		return within_map(s, p);
	}

	const resources::terrain *get_corner(const tile_corners &t, rect_corners c)
	{
		static_assert(std::is_same_v<std::size_t, std::underlying_type_t<rect_corners>>);
		return t[static_cast<std::size_t>(c)];
	}

	resources::transition_tile_type get_transition_type(const std::array<bool, 4u> &arr)
	{
		auto type = uint8{};

		if (arr[static_cast<std::size_t>(rect_corners::top_left)])
			type += 8u;
		if (arr[static_cast<std::size_t>(rect_corners::top_right)])
			type += 1u;
		if (arr[static_cast<std::size_t>(rect_corners::bottom_right)])
			type += 2u;
		if (arr[static_cast<std::size_t>(rect_corners::bottom_left)])
			type += 4u;

		return static_cast<resources::transition_tile_type>(type);
	}

	tile_corners get_terrain_at_tile(const terrain_map &m, tile_position p)
	{
		const auto w = get_width(m);

		//a tile position should always point to the top left of a tile vertex
		const auto top_left = to_1d_index(p, w);
		//if the 1d index of the top left is correct
		//then their should always be a vertex to its right
		assert(top_left < w);
		const auto top_right = top_left + 1u;
		++p.y;
		const auto bottom_left = to_1d_index(p, w);
		const auto bottom_right = bottom_left + 1u;
		//same for the vertex below them, they should be before the end of the vector
		assert(bottom_left < std::size(m.terrain_vertex));

		assert(rect_corners::bottom_right < rect_corners::bottom_left);

		return std::array{
			m.terrain_vertex[top_left],
			m.terrain_vertex[top_right],
			m.terrain_vertex[bottom_right],
			m.terrain_vertex[bottom_left]
		};
	}

	const resources::terrain *get_vertex(const terrain_map &m, terrain_vertex_position p)
	{
		const auto index = to_1d_index(p, get_width(m));
		return m.terrain_vertex[index];
	}

	std::vector<tile_position> get_adjacent_tiles(terrain_vertex_position p)
	{
		return {
			//top row
			{p.x - 1, p.y -1},
			{p.x, p.y -1},
			{p.x + 1, p.y -1},
			//middle row
			{p.x - 1, p.y},
			p,
			{p.x + 1, p.y},
			//bottom row
			{p.x - 1, p.y + 1},
			{p.x, p.y + 1},
			{p.x + 1, p.y + 1},
		};
	}

	std::vector<tile_position> get_adjacent_tiles(const std::vector<terrain_vertex_position> &v)
	{
		auto out = std::vector<tile_position>{};

		for (const auto p : v)
		{
			const auto pos = get_adjacent_tiles(p);
			out.insert(std::end(out), std::begin(pos), std::end(pos));
		}

		remove_duplicates(out, [](tile_position lhs, tile_position rhs) {
			return std::tie(lhs.x, lhs.y) < std::tie(rhs.x, rhs.y);
		});

		return out;
	}

	//NOTE: we cannot know if the tiles have transparent sections 
	// in them, so the terrainmap goes unchanged
	void place_tile(terrain_map &m, tile_position p, const resources::tile &t)
	{
		place_tile(m.tile_layer, p, t);
	}

	void place_tile(terrain_map &m, const std::vector<tile_position> &p, const resources::tile &t) 
	{
		place_tile(m.tile_layer, p, t);
	}

	static void place_terrain_internal(terrain_map &m, terrain_vertex_position p, const resources::terrain *t)
	{
		assert(t);
		const auto index = to_1d_index(p, get_width(m));
		m.terrain_vertex[index] = t;
	}

	static void update_tile_layers_internal(terrain_map &m, const std::vector<tile_position> &positions, const resources::terrain *t)
	{
		//remove tiles from the tile layer,
		//so that they dont obscure the new terrain
		place_tile(m.tile_layer, positions, resources::get_empty_tile());

		const auto end = std::cend(m.terrainset->terrains);
		auto iter = std::begin(m.terrainset->terrains);
		auto iter2 = std::begin(m.terrain_layers);
		assert(std::size(m.terrain_layers) == std::size(m.terrainset->terrains));
		for (iter, iter2; iter != end; ++iter, ++iter2)
		{
			for (const auto p : positions)
			{
				//get the terrain corners for this tile
				const auto corners = get_terrain_at_tile(m, p);
				//get the transition, only looking for terrains bellow the current one
				const auto transition = get_transition_type(corners, iter, end);

				const auto tile = resources::get_random_tile(**iter, transition);
				place_tile(*iter2, p, tile);
			}
		}
	}

	template<typename Position>
	static void update_tile_layers(terrain_map &m, Position p, const resources::terrain *t)
	{
		const auto t_p = get_adjacent_tiles(p);
		update_tile_layers_internal(m, t_p, t);
	}

	void place_terrain(terrain_map &m, terrain_vertex_position p, const resources::terrain *t)
	{
		if (within_map(m, p))
		{
			place_terrain_internal(m, p, t);
			update_tile_layers(m, p, t);
		}
	}

	//positions outside the map will be ignored
	void place_terrain(terrain_map &m, const std::vector<terrain_vertex_position> &positions, const resources::terrain *t)
	{
		const auto s = get_size(m);
		for (const auto p : positions)
			if (within_map(s, p))
				place_terrain_internal(m, p, t);

		update_tile_layers(m, positions, t);
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
		//TODO: return ref to empty tileset?
		// for type == all
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

	static void parse_terrainset(unique_id mod, const data::parser_node &n, data::data_manager &d)
	{
		//terrainsets:
		//	name: [terrains, terrains, terrains]

		auto settings = d.find_or_create<terrain_settings>(resources::get_tile_settings_id(), mod);

		const auto list = n.get_children();

		for (const auto &terrainset_n : list)
		{
			const auto name = terrainset_n->to_string();
			const auto id = d.get_uid(name);

			auto t = d.find_or_create<terrainset>(id, mod);

			const auto seq = terrainset_n->get_child();
			auto unique_list = seq->merge_sequence(t->terrains, [mod, &d](std::string_view s) {
				const auto i = d.get_uid(s);
				return d.find_or_create<terrain>(i, mod);
			});

			std::swap(unique_list, t->terrains);

			settings->terrainsets.emplace_back(t);
		}

		remove_duplicates(settings->terrainsets);
	}

	const terrain_settings *get_terrain_settings()
	{
		return data::get<terrain_settings>(id::terrain_settings);
	}

	const terrain *get_empty_terrain()
	{
		const auto settings = get_terrain_settings();
		return settings->empty_terrain;
	}

	std::vector<tile>& get_transitions(terrain &t, transition_tile_type ty)
	{
		return get_transition(ty, t);
	}

	const std::vector<tile>& get_transitions(const terrain &t, transition_tile_type ty)
	{
		return get_transition<const std::vector<tile>>(ty, t);
	}

	tile get_random_tile(const terrain &t, transition_tile_type type)
	{
		const auto vec = get_transitions(t, type);
		return random_element(std::begin(vec), std::end(vec));
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
