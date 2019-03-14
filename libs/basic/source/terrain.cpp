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
		d.register_resource_type("terrain"sv, resources::parse_terrain);
		d.register_resource_type("terrainsets"sv, resources::parse_terrainset);
	}

	static terrain_count_t get_terrain_index(const resources::terrainset *set, const resources::terrain *t)
	{
		assert(set && t);

		for (auto i = terrain_count_t{}; i < set->terrains.size(); ++i)
			if (set->terrains[i] == t)
				return i;

		throw terrain_error{"tried to index a terrain that isn't in this terrain set"};
	}

	static tile_corners make_empty_corners()
	{
		const auto empty = resources::get_empty_terrain();
		return std::array{ empty, empty, empty, empty };
	}

	static tile_corners get_terrain_at_tile(const std::vector<const resources::terrain*> &v, terrain_count_t w, tile_position p)
	{
		auto out = make_empty_corners();

		const auto map_size = terrain_vertex_position{ 
			signed_cast(w),
			signed_cast(std::size(v) / w)
		};

		if (!within_map(map_size - terrain_vertex_position{ 1, 1 }, p))
			return out;
		
		//calculate vertex indicies
		const auto top_left_i = to_1d_index(p, w);
		const auto top_right_i = top_left_i + 1u;
		const auto bottom_left_i = top_left_i + w;
		const auto bottom_right_i = bottom_left_i + 1u;

		//top left vertex is always within the map
		out[static_cast<std::size_t>(rect_corners::top_left)] = v[top_left_i];

		//only override other vertecies if they are within the map
		if(p.x < signed_cast(w))
			out[static_cast<std::size_t>(rect_corners::top_right)] = v[top_right_i];

		if (p.y < map_size.y)
			out[static_cast<std::size_t>(rect_corners::bottom_left)] = v[bottom_left_i];

		if (p.x < signed_cast(w) && p.y < map_size.y)
			out[static_cast<std::size_t>(rect_corners::bottom_right)] = v[bottom_right_i];

		return out;
	}

	template<typename InputIt>
	static tile_map generate_layer(const std::vector<const resources::terrain*> &v, terrain_count_t w, InputIt first, InputIt last)
	{
		auto out = tile_map{};
		out.width = w - 1;
		out.tilesets = std::vector<const resources::tileset*>{
			resources::get_empty_terrain(),
			*first
		};

		const auto tile_w = w - 1;

		for (auto i = std::size_t{}; i < std::size(v); ++i)
		{
			const auto p = to_2d_index(i, w);
			if (p.first == tile_w)
				continue;

			const auto pos = tile_position{
				signed_cast(p.first) - 1,
				signed_cast(p.second) - 1
			};

			const auto corners = get_terrain_at_tile(v, w, pos);
			const auto type = get_transition_type(corners, first, last);
			const auto tile = resources::get_random_tile(**first, type);
			out.tiles.emplace_back(get_tile_id(out, tile));
		}

		return out;
	}

	static std::vector<tile_map> generate_terrain_layers(const resources::terrainset *t, const std::vector<const resources::terrain*> &v, terrain_count_t width)
	{
		auto out = std::vector<tile_map>{};

		const auto end = std::cend(t->terrains);
		for (auto iter = std::cbegin(t->terrains); iter != end; ++iter)
			out.emplace_back(generate_layer(v, width, iter, end));

		return out;
	}

	bool is_valid(const raw_terrain_map &r)
	{
		if (std::empty(r.tile_layer.tiles) ||
			r.tile_layer.width < 1)
		{
			LOGWARNING("raw terrain map must have a valid tile layer in ordre to compute size");
			return false;
		}

		//check the the map has a valid terrain vertex
		const auto size = get_size(r.tile_layer);
		const auto terrain_size = terrain_vertex_position{
			signed_cast(size.x + 1u),
			signed_cast(size.y + 1u)
		};

		const auto vertex_length = terrain_size.x * terrain_size.y;

		if (!std::empty(r.terrain_vertex) &&
			std::size(r.terrain_vertex) != vertex_length)
		{
			LOGWARNING("raw map terrain vertex list should be the same number of tiles as the tile_layer, or it should be empty.");
			return false;
		}

		if (!std::empty(r.terrain_layers))
		{
			if (std::any_of(std::begin(r.terrain_layers), std::end(r.terrain_layers), [size](auto &&l) {
				return get_size(l) != size;
			}))
			{
				LOGWARNING("raw terrain map, if tile data for terrain layers is present, then those layers must be the same size as the tile-layer");
				return false;
			}
		}

		return true;
	}

	bool is_valid(const raw_terrain_map &r, vector_int level_size, resources::tile_size_t tile_size)
	{
		if (!is_valid(r))
			return false;

		//check the scale and size of the map is correct for the level size
		const auto size = tile_position{
			signed_cast(level_size.x / tile_size),
			signed_cast(level_size.y / tile_size)
		};

		const auto tilemap_size = get_size(r.tile_layer);

		if (tilemap_size != size)
		{
			LOGWARNING("loaded map size must be a multiple of tile size: [" +
				to_string(tile_size) + "], level will be adjusted to a valid value");

			return false;
		}

		return true;
	}

	terrain_map to_terrain_map(const raw_terrain_map &r)
	{
		if(!is_valid(r))
			throw terrain_error{ "raw terrain map is not valid" };

		auto m = terrain_map{};

		m.terrainset = data::get<resources::terrainset>(r.terrainset);
		
		//tile layer is required to be valid
		m.tile_layer = to_tile_map(r.tile_layer);
		const auto size = get_size(m.tile_layer);

		const auto empty = resources::get_empty_terrain();
		//if the terrain_vertex isn't present, then fill with empty
		if (std::empty(m.terrain_vertex))
			m.terrain_vertex = std::vector<const resources::terrain*>((size.x + 1) * (size.y + 1), empty);
		else
		{
			std::transform(std::begin(r.terrain_vertex), std::end(r.terrain_vertex),
				std::back_inserter(m.terrain_vertex), [empty, &terrains = m.terrainset->terrains](terrain_count_t t) {
				if (t == terrain_count_t{})
					return empty;

				assert(t <= std::size(terrains));
				return terrains[t - 1u];
			});
		}

		// if the terrain layers are empty then generate them
		if (std::empty(r.terrain_layers))
		{
			m.terrain_layers = generate_terrain_layers(m.terrainset, m.terrain_vertex, size.x + 1);
		}
		else
		{
			std::transform(std::begin(r.terrain_layers), std::end(r.terrain_layers),
				std::back_inserter(m.terrain_layers), to_tile_map);
		}

		//if we dont have the correct number of terrain_layers
		if (std::size(m.terrainset->terrains) != std::size(m.terrain_layers))
			throw terrain_error{ "terrain map missing some terrain layers, or has too many" };

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

		auto map = terrain_map{};
		map.terrainset = terrainset;
		map.terrain_vertex = std::vector<const resources::terrain*>((size.x + 1) * (size.y + 1), t);

		const auto empty_layer = make_map(size, resources::get_empty_tile());

		if (t != resources::get_empty_terrain())
		{
			//fill in the correct terrain layer
			const auto index = get_terrain_index(terrainset, t);

			const auto end = std::size(terrainset->terrains);
			for (auto i = std::size_t{}; i < end; ++i)
			{
				if (i == index)
				{
					map.terrain_layers.emplace_back(make_map(
						size,
						resources::get_random_tile(*terrainset->terrains[i],
							resources::transition_tile_type::all)
					));
				}
				else
					map.terrain_layers.emplace_back(empty_layer);
			}
		}
		else
		{
			//otherwise all layers can be blank
			map.terrain_layers = std::vector<tile_map>(std::size(terrainset->terrains), empty_layer);
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
			p.y < 0 )
			return false;

		if(p.x < s.x &&
			p.y < s.y)
			return true;

		return false;
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

		return get_terrain_at_tile(m.terrain_vertex, w, p);
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

	static void update_tile_layers_internal(terrain_map &m, const std::vector<tile_position> &positions)
	{
		//remove tiles from the tile layer,
		//so that they dont obscure the new terrain
		place_tile(m.tile_layer, positions, resources::get_empty_tile());

		const auto end = std::cend(m.terrainset->terrains);
		auto terrain_iter = std::begin(m.terrainset->terrains);
		auto layer_iter = std::begin(m.terrain_layers);
		assert(std::size(m.terrain_layers) == std::size(m.terrainset->terrains));
		for (terrain_iter, layer_iter; terrain_iter != end; ++terrain_iter, ++layer_iter)
		{
			assert(layer_iter != std::end(m.terrain_layers));
			for (const auto p : positions)
			{
				//get the terrain corners for this tile
				const auto corners = get_terrain_at_tile(m, p);

				const auto equal_terrain = [t = *terrain_iter](auto &&other_t){
					return t == other_t;
				};

				//get the transition, only looking for terrains bellow the current one
				const auto transition = [&] {
					if (std::none_of(std::begin(corners), std::end(corners), equal_terrain))
						return resources::transition_tile_type::none;
					else
						return get_transition_type(corners, terrain_iter, end);
				}();

				const auto tile = resources::get_random_tile(**terrain_iter, transition);
				place_tile(*layer_iter, p, tile);
			}
		}
	}

	template<typename Position>
	static void update_tile_layers(terrain_map &m, Position p)
	{
		const auto t_p = get_adjacent_tiles(p);
		update_tile_layers_internal(m, t_p);
	}

	void place_terrain(terrain_map &m, terrain_vertex_position p, const resources::terrain *t)
	{
		if (within_map(m, p))
		{
			place_terrain_internal(m, p, t);
			update_tile_layers(m, p);
		}
	}

	//positions outside the map will be ignored
	void place_terrain(terrain_map &m, const std::vector<terrain_vertex_position> &positions, const resources::terrain *t)
	{
		const auto s = get_size(m);
		for (const auto p : positions)
			if (within_map(s, p))
				place_terrain_internal(m, p, t);

		update_tile_layers(m, positions);
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
		if constexpr (std::is_const_v<U>)
		{
			if (type == none)
				return get_empty_terrain()->tiles;
		}
		else
			assert(type != none);

		//we store the 'all' tiles in the 'none' index
		//since it is unused
		if (type == all)
			type = none;
		assert(type < std::size(t.terrain_transition_tiles));
		return t.terrain_transition_tiles[type];
	}

	static void add_tiles_to_terrain(terrain &terrain, const vector_int start_pos, const hades::resources::texture *tex,
		const std::vector<transition_tile_type> &tiles, const tile_count_t tiles_per_row, const tile_size_t tile_size)
	{
		auto count = tile_size_t{};
		for (auto &t : tiles)
		{
			assert(t <= transition_end);

			const auto tile_pos = vector_t{
				(count % tiles_per_row) * tile_size + start_pos.x,
				(count / tiles_per_row) * tile_size + start_pos.y
			};

			++count;

			if (t == none)
			{
				LOGWARNING("skipping terrain tile, layouts cannot contain the 'none' transition");
				continue;
			}

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

		if (tiles_per_row < 0)
		{
			LOGERROR("a terrain group must provide tiles-per-row");
		}

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
		//				texture: <// texture to draw the tiles from; required
		//				left: <// pixel start of tileset; default: 0
		//				top: <// pixel left of tileset; default: 0
		//				tiles_per_row: <// number of tiles per row; required
		//				tile_count: <// total amount of tiles in tileset; default: tiles_per_row
		//			}
		//		terrain:
		//			- {
		//				texture: <// as above
		//				left: <// as above
		//				top: <// as above
		//				tiles-per-row: <// as above
		//				tile_count: <// optional; default is the length of layout
		//				layout: //either war3, a single unique_id, or a set of tile_count unique_ids; required
		//						// see parse_layout()::transition_names for a list of unique_ids
		//			}

		const auto terrains_list = p.get_children();
		auto settings = d.get<resources::terrain_settings>(id::terrain_settings);
		assert(settings);

		const auto tile_size = settings->tile_size;

		for (const auto &terrain_node : terrains_list)
		{
			using namespace std::string_view_literals;
			const auto name = terrain_node->to_string();
			const auto id = d.get_uid(name);

			auto terrain = d.find_or_create<resources::terrain>(id, m);
			assert(terrain);
			//parse_tiles will fill in the tags and tiles
			resources::detail::parse_tiles(m, *terrain, tile_size, *terrain_node, d);

			const auto terrain_n = terrain_node->get_child("terrain"sv);
			if (terrain_n)
			{
				const auto terrain_group = terrain_n->get_children();
				for (const auto &group : terrain_group)
					parse_terrain_group(m, *terrain, *group, d, tile_size);
			}

			settings->tilesets.emplace_back(terrain);
			settings->terrains.emplace_back(terrain);
		}

		remove_duplicates(settings->terrains);
		remove_duplicates(settings->tilesets);
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

			auto unique_list = terrainset_n->merge_sequence(t->terrains, [mod, &d](std::string_view s) {
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
