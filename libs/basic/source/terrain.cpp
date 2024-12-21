#include "hades/terrain.hpp"

#include <execution>
#include <map>
#include <span>
#include <string>

#include "hades/data.hpp"
#include "hades/deflate.hpp"
#include "hades/parser.hpp"
#include "hades/random.hpp"
#include "hades/table.hpp"
#include "hades/tiles.hpp"
#include "hades/triangle_math.hpp"
#include "hades/writer.hpp"

using namespace std::string_literals;
using namespace std::string_view_literals;

auto background_terrain_id = hades::unique_zero;
auto editor_terrainset_id = hades::unique_zero;

namespace hades::resources
{
	static void parse_terrain(unique_id, const data::parser_node&, data::data_manager&);
	static void parse_terrainset(unique_id, const data::parser_node&, data::data_manager&);
	static void parse_terrain_settings(unique_id, const data::parser_node&, data::data_manager&);
	static void parse_cliff_styles(unique_id, const data::parser_node&, data::data_manager&);
}

namespace hades
{
	namespace detail
	{
		static make_texture_link_f make_texture_link{};
	}

	namespace id
	{
		static unique_id terrain_settings = unique_id::zero;
	}

	template<typename Func>
	static void apply_to_terrain(resources::terrain &t, Func&& func)
	{
		// tileset tiles
		std::invoke(func, t.tiles);
		// terrain tiles
		std::for_each(begin(t.terrain_transition_tiles), end(t.terrain_transition_tiles), std::forward<Func>(func));
		return;
	}

	void register_terrain_resources(data::data_manager &d)
	{
		register_terrain_resources(d,
			[](auto&, auto, auto)->resources::resource_link<resources::texture> {
				return {};
			});
	}

	constexpr auto terrain_settings_str = "terrain-settings"sv;
	constexpr auto terrainsets_str = "terrainsets"sv;
	constexpr auto terrains_str = "tilesets"sv;
	constexpr auto cliff_styles_str = "cliff-types"sv;
	constexpr auto empty_terrainset_str = "air-terrainset"sv;

	void register_terrain_resources(data::data_manager &d, detail::make_texture_link_f func)
	{
		detail::make_texture_link = func;
		using namespace hades::resources;

		//create the terrain settings
		//and empty terrain first,
		//so that tiles can use them as tilesets without knowing
		//that they're really terrains
		id::terrain_settings = d.get_uid(get_tile_settings_name());
		auto terrain_settings_res = d.find_or_create<terrain_settings>(id::terrain_settings, {}, terrain_settings_str);
		const auto empty_tileset_id = d.get_uid(get_empty_tileset_name());
		auto empty = d.find_or_create<terrain>(empty_tileset_id, {}, terrains_str);
		
		//fill all the terrain tile lists with a default constructed tile
		apply_to_terrain(*empty, [empty](std::vector<tile>&v) {
			v.emplace_back(resource_link<texture>{}, tile_size_t{}, tile_size_t{}, empty);
		});
		
		terrain_settings_res->empty_terrain = d.make_resource_link<terrain>(empty_tileset_id, id::terrain_settings);
		terrain_settings_res->empty_tileset = d.make_resource_link<tileset>(empty_tileset_id, id::terrain_settings);

		const auto empty_tset_id = d.get_uid(empty_terrainset_str);
		auto empty_tset = d.find_or_create<terrainset>(empty_tset_id, {}, terrainsets_str);
		empty_tset->terrains.emplace_back(terrain_settings_res->empty_terrain);

		terrain_settings_res->empty_terrainset = d.make_resource_link<terrainset>(empty_tset_id, id::terrain_settings);

		// the exact same as empty tile
		// since its now used as 'air' tiles for higher layers
		// it can be made unpathable by games, so that unwalkable
		// background can be visible through the map
		background_terrain_id = d.get_uid("terrain-background"sv);
		auto background_terrain = d.find_or_create<terrain>(background_terrain_id, {}, terrains_str);
		
		// add empty tiles to all the tile arrays
		apply_to_terrain(*background_terrain, [background_terrain](std::vector<tile>& v) {
			v.emplace_back(resource_link<texture>{}, tile_size_t{}, tile_size_t{}, background_terrain);
			return;
		});

		terrain_settings_res->background_terrain = d.make_resource_link<terrain>(background_terrain_id, id::terrain_settings);

		//register tile resources
		register_tiles_resources(d, func);

		//replace the tileset and tile settings parsers
		//register tile resources
		d.register_resource_type(get_tile_settings_name(), parse_terrain_settings);
		d.register_resource_type(terrain_settings_str, parse_terrain_settings);
		d.register_resource_type(get_tilesets_name(), parse_terrain);
		d.register_resource_type(terrainsets_str, parse_terrainset);
		d.register_resource_type(cliff_styles_str, parse_cliff_styles);
	}

	static terrain_id_t get_terrain_id(const resources::terrainset *set, const resources::terrain *t)
	{
		assert(set && t);

		const auto terrain_size = size(set->terrains);
		for (auto i = std::size_t{}; i < terrain_size; ++i)
			if (set->terrains[i].get() == t)
				return integer_cast<terrain_id_t>(i);

		throw terrain_error{ "tried to index a terrain that isn't in this terrain set"s };
	}

	static tile_corners make_empty_corners(const resources::terrain_settings& s)
	{
		const auto empty = s.empty_terrain.get();
		return std::array{ empty, empty, empty, empty };
	}

    static tile_corners get_terrain_at_tile(const std::vector<const resources::terrain*> &v,
		const terrain_index_t width, tile_position p, const resources::terrain_settings& s)
	{
		auto out = make_empty_corners(s);

        const auto w = integer_cast<std::size_t>(width);
		const auto map_size = terrain_vertex_position{ 
			width,
            integer_cast<terrain_vertex_position::value_type>(std::size(v) / w)
		};

		if (!within_map(map_size - terrain_vertex_position{ 1, 1 }, p))
			return out;
		
		//calculate vertex indicies
		const auto top_left_i = to_1d_index(p, w);
        const auto top_right_i = top_left_i + 1;
		const auto bottom_left_i = top_left_i + w;
        const auto bottom_right_i = bottom_left_i + 1;

		//top left vertex is always within the map
		out[static_cast<std::size_t>(rect_corners::top_left)] = v[top_left_i];

		//only override other verticies if they are within the map
		if(p.x < signed_cast(w))
			out[static_cast<std::size_t>(rect_corners::top_right)] = v[top_right_i];

		if (p.y < map_size.y)
			out[static_cast<std::size_t>(rect_corners::bottom_left)] = v[bottom_left_i];

		if (p.x < signed_cast(w) && p.y < map_size.y)
			out[static_cast<std::size_t>(rect_corners::bottom_right)] = v[bottom_right_i];

		return out;
	}

	using make_terrain_index_map_return = std::array<std::vector<tile_id_t>, enum_type(resources::transition_tile_type::transition_end)>;
	static make_terrain_index_map_return make_terrain_index_map(tile_map& map, const resources::terrain& terr, const resources::terrain_settings& s)
	{
		using tile_type = resources::transition_tile_type;
		auto tile_ids = make_terrain_index_map_return{};
		for (auto type = tile_type::transition_begin; type != tile_type::transition_end; type = next(type))
		{
			if (type == tile_type::none)
				continue;
			auto& v = tile_ids[enum_type(type)];
			const auto& tiles = resources::get_transitions(terr, type, s);
			v.reserve(size(tiles));
			std::ranges::transform(tiles, std::back_inserter(v), [&](const resources::tile& t) {
				return make_tile_id(map, t, &terr);
				});
		}
		return tile_ids;
	}

	template<typename InputIt>
	static tile_map generate_layer(const std::vector<const resources::terrain*> &verts,
		tile_index_t w, InputIt first, InputIt last, const resources::terrain_settings& s)
	{
		//NOTE: w2 and h are 0 based, and one less than vertex width and height
		//this is good, it means they are equal to the tile width/height
		assert(!std::empty(verts));
		const auto vertex_h = integer_cast<tile_index_t>(size(verts)) / (w + 1);
		const auto h = vertex_h - 1;

		auto out = tile_map{};
		out.width = w;
		out.tilesets = std::vector<const resources::tileset*>{
			resources::get_empty_terrain(s),
			resources::get_error_tileset(s),
			first->get()
		};

		const auto error_id = get_tile_id(out, resources::get_error_tile(s));
		const auto empty_id = get_tile_id(out, resources::get_empty_tile(s));

		const auto tile_ids = make_terrain_index_map(out, **first, s);

		const auto size = w * h;
		out.tiles.resize(size, empty_id);

		using tile_type = resources::transition_tile_type;
		// TODO: some way to parallelize this; we need access to the index
		//		so it might not be possible using a std solution
		for (auto i = tile_index_t{}; i < size; ++i)
		{
			const auto [x, y] = to_2d_index(i, w);
			assert(x < w);

			const auto corners = get_terrain_at_tile(verts, w + 1, { integer_cast<int32>(x), integer_cast<int32>(y) }, s);
			const auto type = get_transition_type(corners, first, last);
			if (type == tile_type::none)
			{
				out.tiles[i] = empty_id;
				continue;
			}

			const auto& tiles = tile_ids[enum_type(type)];
			if (empty(tiles))
			{
				out.tiles[i] = error_id;
				continue;
			}

			const auto tile = *random_element(begin(tiles), end(tiles));
			out.tiles[i] = tile;
		}

		return out;
	}

	static std::vector<tile_map> generate_terrain_layers(const resources::terrainset *t,
		const std::vector<const resources::terrain*> &v, tile_index_t width, const resources::terrain_settings& s)
	{
		auto out = std::vector<tile_map>{};
		out.reserve(size(t->terrains));

		const auto end = std::crend(t->terrains);
		// TODO: good candidate for parallel
		for (auto iter = std::crbegin(t->terrains); iter != end; ++iter)
			out.emplace_back(generate_layer(v, width, iter, end, s));

		std::ranges::reverse(out);

		return out;
	}

	static tile_map generate_cliff_layer(const terrain_map& map,
		const tile_position world_size, const resources::terrain_settings& s)
	{
		const auto size = world_size.x * world_size.y * enum_type(terrain_map::cliff_layer_layout::multiplier);
		auto out = tile_map{};
		const auto empty_terrain = resources::get_empty_terrain(s);
		const auto error_terrain = resources::get_error_tileset(s);
		const auto cliff_type = map.terrainset->cliff_type.get();
		const auto face_terrain = cliff_type->cliff_face_terrain.get();
		const auto surface_terrain = cliff_type->cliff_surface_terrain.get();

		out.tilesets = std::vector<const resources::tileset*>{
			empty_terrain,
			error_terrain,
			face_terrain,
			surface_terrain,
		};

		const auto error_id = get_tile_id(out, resources::get_error_tile(s));
		const auto empty_id = get_tile_id(out, resources::get_empty_tile(s));

		const auto face_tiles = make_terrain_index_map(out, *face_terrain, s);
		const auto surface_tiles = make_terrain_index_map(out, *surface_terrain, s);
		
		out.tiles.resize(size, empty_id);
		out.width = size;
		
		// TODO: invert this into a basic loop
		for_each_position_rect({}, world_size, world_size, [&](const tile_position pos) {
			using cliff_layout = terrain_map::cliff_layer_layout;
			const auto index = integer_cast<std::size_t>(to_tile_index(pos, map)) * enum_type(cliff_layout::multiplier);
			const auto indicies = std::array<std::size_t, 4>{
				index + enum_type(cliff_layout::top),
				index + enum_type(cliff_layout::right),
				index + enum_type(cliff_layout::bottom),
				index + enum_type(cliff_layout::left)
			};

			const auto surface_index = index + enum_type(cliff_layout::surface);

			const auto cliffs = get_adjacent_cliffs(pos, map);
			for (auto edge = rect_edges::begin; edge != rect_edges::end; edge = next(edge))
			{
				if (cliffs.test(enum_type(edge)))
				{
					const auto i = indicies[enum_type(edge)];
					auto& tile_list = face_tiles[enum_type(resources::transition_tile_type::all)];
					if (!empty(tile_list))
					{
						const auto tile_id = *random_element(begin(tile_list), end(tile_list));
						out.tiles[i] = tile_id;
					}
				}
			}

			const auto cliff_corners = get_cliff_corners(pos, map);
			const auto surface_tile = get_transition_type(cliff_corners);
			if (surface_tile == resources::transition_tile_type::none)
				return;

			auto& tile_list = face_tiles[enum_type(surface_tile)];
			if (empty(tile_list))
			{
				out.tiles[surface_index] = error_id;
			}

			const auto tile_id = *random_element(begin(tile_list), end(tile_list));
			out.tiles[surface_index] = tile_id;
		});

		return out;
	}

	static tile_position get_size(const raw_terrain_map& t)
	{
		if (t.width == 0 || t.terrain_vertex.empty())
			throw terrain_error{ "malformed raw_terrain_map"s };

		return {
			integer_cast<tile_position::value_type>(t.width - 1),
			integer_cast<tile_position::value_type>(integer_cast<tile_index_t>(t.terrain_vertex.size()) / t.width - 1)
		};
	}

	bool is_valid(const raw_terrain_map &r)
	{
		if (r.width == bad_tile_index)
		{
			log_error("missing vertex width"sv);
			return false;
		}

		auto size = bad_tile_position;
		try
		{
			//check the the map has a valid terrain vertex
			size = get_size(r);
		}
		catch (const terrain_error& e)
		{
			log_error(e.what());
			return false;
		}

		const auto terrain_size = terrain_vertex_position{
            size.x + 1,
            size.y + 1
		};

		const auto vertex_length = integer_cast<std::size_t>(terrain_size.x) * integer_cast<std::size_t>(terrain_size.y);
		if (vertex_length != std::size(r.terrain_vertex))
		{
			log_error("Malformed vertex layer in terrain"sv);
			return false;
		}

		if (std::size(r.heightmap) != vertex_length)
		{
			LOGERROR("Heightmap must have a sample for each vertex"sv);
			return false;
		}

		const auto tile_count = integer_cast<std::size_t>(size.x) * integer_cast<std::size_t>(size.y);
		if (std::size(r.cliff_layer) != tile_count)
		{
			log_error("Cliff layer must have a sample for each tile"sv);
			return false;
		}

		if (std::size(r.ramp_layer) != tile_count)
		{
			log_error("Ramp layer must have a sample for each tile"sv);
			return false;
		}

		// if cliff tiles are present, they must be the correct size
		if (!std::empty(r.cliff_tiles.tiles) && std::size(r.cliff_tiles.tiles) != tile_count * enum_type(terrain_map::cliff_layer_layout::multiplier))
		{
			log_error("Cliff tilemap must have 5 samples per tile"sv);
			return false;
		}

		if (!std::empty(r.terrain_layers))
		{
			if (std::any_of(std::begin(r.terrain_layers), std::end(r.terrain_layers), [size](auto &&l) {
				return get_size(l) != size;
			}))
			{
				LOGERROR("raw terrain map, if tile data for terrain layers is present, then those layers must be the correct size");
				return false;
			}
		}

		return true;
	}

	bool is_valid(const raw_terrain_map &r, vector2_int level_size, resources::tile_size_t tile_size)
	{
		if (!is_valid(r))
			return false;

		//check the scale and size of the map is correct for the level size
		const auto size = tile_position{
			signed_cast(level_size.x / tile_size),
			signed_cast(level_size.y / tile_size)
		};

		const auto tilemap_size = get_size(r);

		if (tilemap_size != size)
		{
			LOGWARNING("loaded map size must be a multiple of tile size: [" +
				to_string(tile_size) + "], level will be adjusted to a valid value");

			return false;
		}

		return true;
	}

	constexpr auto terrainset_str = "terrainset"sv;
	constexpr auto terrain_vertex_str = "terrain-vertex"sv;
	constexpr auto terrain_height_str = "vertex-height"sv;
	constexpr auto terrain_ramp_str = "ramp-layer"sv;
	constexpr auto terrain_cliff_layer_str = "cliff-layer"sv;
	constexpr auto terrain_layers_str = "terrain-layers"sv;
	constexpr auto terrain_cliff_tiles_str = "cliff-tiles"sv;
	constexpr auto terrain_vertex_width_str = "width"sv;

	void write_raw_terrain_map(const raw_terrain_map & m, data::writer & w)
	{
		// we start in a terrain: node

		//terrain:
		//	terrainset:
		//	terrain_vertex:
		//	vertex_height:
		//	ramp-layer:
		//	terrain_layers:
		//  cliff_layers:
		//	cliff-tiles:
		//	width:

		w.write(terrainset_str, m.terrainset);

		{
			const auto compressed = zip::deflate(m.terrain_vertex);
			w.write(terrain_vertex_str, base64_encode(compressed));
		}
		{
			const auto compressed_height = zip::deflate(m.heightmap);
			w.write(terrain_height_str, base64_encode(compressed_height));
		}
		{
			const auto compressed_cliff_layer = zip::deflate(m.cliff_layer);
			w.write(terrain_cliff_layer_str, base64_encode(compressed_cliff_layer));
		}
		{ 
			const auto compressed_ramp_layer = zip::deflate(m.ramp_layer);
			w.write(terrain_ramp_str, base64_encode(compressed_ramp_layer));
		}

		// write terrain tile layers
		w.start_sequence(terrain_layers_str); 
		for (const auto &l : m.terrain_layers)
		{
			w.start_map();
			write_raw_map(l, w);
			w.end_map();
		}
		w.end_sequence();

		w.start_map("cliff-tiles"sv);
		write_raw_map(m.cliff_tiles, w);
		w.end_map();

		w.write(terrain_vertex_width_str, m.width);
	}

	namespace detail
	{
		template<typename Ty>
		void parse_compressed_sequence(const data::parser_node& p, std::string_view target_str, std::vector<Ty>& out, std::size_t expected_size = {})
		{
			if (auto node = p.get_child(target_str);
				node)
			{
				if (node->is_sequence())
				{
					auto sequence_ret = node->to_sequence<Ty>();
					out = std::move(sequence_ret);
				}
				else
				{
					const auto encoded = node->to_string();
					const auto bytes = base64_decode<std::byte>(encoded);
					auto inflated = zip::inflate<Ty>(bytes, expected_size * sizeof(Ty));
					out = std::move(inflated);
				}
			}

			return;
		}
	}

	raw_terrain_map read_raw_terrain_map(const data::parser_node &p, std::size_t layer_size, std::size_t vert_size)
	{
		//terrain:
		//	terrainset:
		//	terrain_vertex:
		//	vertex_height:
		//	ramp-layer:
		//	terrain_layers:
		//  cliff_layers:
		//	cliff-tiles:
		//	width:

		raw_terrain_map out [[indeterminate]];

		out.terrainset = data::parse_tools::get_unique(p, terrainset_str, unique_zero);

		detail::parse_compressed_sequence(p, terrain_vertex_str, out.terrain_vertex, vert_size);
		detail::parse_compressed_sequence(p, terrain_height_str, out.heightmap, vert_size);
		detail::parse_compressed_sequence(p, terrain_cliff_layer_str, out.cliff_layer, layer_size);
		detail::parse_compressed_sequence(p, terrain_ramp_str, out.ramp_layer, layer_size);

		auto layers = std::vector<raw_map>{};
		const auto layer_node = p.get_child(terrain_layers_str);
		for (const auto& l : layer_node->get_children())
			layers.emplace_back(read_raw_map(*l, layer_size));
		out.terrain_layers = std::move(layers);

		const auto cliff_tile_node = p.get_child(terrain_cliff_tiles_str);
		if(cliff_tile_node)
			out.cliff_tiles = read_raw_map(*cliff_tile_node, layer_size * 5);

		out.width = data::parse_tools::get_scalar<terrain_index_t>(p, terrain_vertex_width_str, bad_tile_index);

		return out;
	}

	template<typename Raw>
		requires std::same_as<raw_terrain_map, std::decay_t<Raw>>
	terrain_map to_terrain_map_impl(Raw &&r, const resources::terrain_settings& settings)
	{
		if (!is_valid(r))
			throw terrain_error{ "raw terrain map is not valid" };

		auto m = terrain_map{};

		m.terrainset = data::get<resources::terrainset>(r.terrainset);
		m.heightmap = std::forward<Raw>(r).heightmap;
		m.cliff_layer = std::forward<Raw>(r).cliff_layer;
		m.ramp_layer = std::forward<Raw>(r).ramp_layer;
		
		const auto size = get_size(r);
		const auto tile_count = size.x * size.y;
		using ramp_layer_t = terrain_map::ramp_layer_t;
		if (std::size(m.ramp_layer) != tile_count)
			throw terrain_error{ "Malformed raw terrain map"s }; 
		if(std::size(m.cliff_layer) != tile_count)
			throw terrain_error{ "Malformed raw terrain map"s };
		if(std::size(r.terrain_vertex) != std::size(m.heightmap))
			throw terrain_error{ "Malformed raw terrain map"s };

		const auto empty = resources::get_empty_terrain(settings);
		const auto error = resources::get_error_tileset(settings);
		
		if (std::empty(r.terrain_vertex))
			throw terrain_error{ "Malformed raw terrain map"s };
		else
		{
			m.terrain_vertex.resize(std::size(r.terrain_vertex), nullptr);
			std::transform(std::execution::par_unseq, std::begin(r.terrain_vertex), std::end(r.terrain_vertex),
				std::begin(m.terrain_vertex), [empty, &terrains = m.terrainset->terrains](terrain_id_t t) {
					if (t == terrain_id_t{})
						return empty;

					assert(t <= std::size(terrains));
					return terrains[t - 1u].get();
				});
		}

		if (std::empty(r.terrain_layers))
			m.terrain_layers = generate_terrain_layers(m.terrainset, m.terrain_vertex, size.x, settings);
		else
		{
			// TODO: we need a way to just iterate through the expected layers generating them as needed
			//			copy generate_terrain_layers but with modification
			//m.terrain_layers.resize(size(m.terrainset));
			m.terrain_layers = generate_terrain_layers(m.terrainset, m.terrain_vertex, size.x, settings);
			for (auto& l : m.terrain_layers)
			{
				const auto terrain = std::find_if(begin(l.tilesets), end(l.tilesets), [empty, error](auto& t) {
					return t != empty && t != error;
				});
				assert(terrain != end(l.tilesets));

				const auto raw_layer = std::find_if(begin(r.terrain_layers), end(r.terrain_layers), [t = (*terrain)->id](auto& l) {
					return std::any_of(begin(l.tilesets), end(l.tilesets), [t](auto& tileset) {
						return std::get<unique_id>(tileset) == t;
						});
					});
				
				if (raw_layer == end(r.terrain_layers))
				{
					//missing layer
					//a new layer will have already been generated
					continue;
				}

				l = to_tile_map(*raw_layer);
			}
		}

		m.width = r.width;
		if (std::empty(r.cliff_tiles.tiles))
			m.cliff_tiles = generate_cliff_layer(m, size, settings);
		else
			m.cliff_tiles = to_tile_map(r.cliff_tiles);

		const auto cliff_tile_count = tile_count * enum_type(terrain_map::cliff_layer_layout::multiplier);
		if (std::size(m.cliff_tiles.tiles) != cliff_tile_count)
		{
			const auto& empty_tile = resources::get_empty_tile(settings);
			const auto empty_id = make_tile_id(m.cliff_tiles, empty_tile, empty);
			m.cliff_tiles.tiles.resize(cliff_tile_count, empty_id);
			m.cliff_tiles.width = cliff_tile_count;
		}

		assert(is_valid(to_raw_terrain_map(m, settings)));

		return m;
	}

	terrain_map to_terrain_map(const raw_terrain_map &r, const resources::terrain_settings& s)
	{
		return to_terrain_map_impl(r, s);
	}

	terrain_map to_terrain_map(raw_terrain_map&& r, const resources::terrain_settings& s)
	{
		return to_terrain_map_impl(std::move(r), s);
	}

	template<typename Map>
		requires std::same_as<terrain_map, std::decay_t<Map>>
	static raw_terrain_map to_raw_terrain_map_impl(Map &&t, const resources::terrain_settings& s)
	{
		auto m = raw_terrain_map{};

		m.terrainset = t.terrainset->id;
		m.heightmap = std::forward<Map>(t).heightmap;
		m.cliff_layer = std::forward<Map>(t).cliff_layer;
		m.ramp_layer = std::forward<Map>(t).ramp_layer;
		m.cliff_tiles = to_raw_map(std::forward<Map>(t).cliff_tiles);

		//build a replacement lookup table
		auto t_map = std::vector<std::tuple<const resources::terrain*, terrain_id_t>>{};
		t_map.reserve(size(t.terrainset->terrains));
		const auto empty = resources::get_empty_terrain(s);
		//add the empty terrain with the index 0
		t_map.emplace_back(empty, terrain_id_t{});

		//add the rest of the terrains, offset the index by 1
		for (auto i = terrain_id_t{}; i < std::size(t.terrainset->terrains); ++i)
			t_map.emplace_back(t.terrainset->terrains[i].get(), i + 1u);

		m.terrain_vertex.resize(size(t.terrain_vertex), std::numeric_limits<terrain_id_t>::max());
		std::ranges::transform(t.terrain_vertex, std::begin(m.terrain_vertex),
			[&t_map](const resources::terrain* t) {
				for (auto& [terr, id] : t_map)
				{
					if (terr == t)
						return id;
				}
				// TODO: throw
				return std::numeric_limits<terrain_id_t>::max();
			});

		for (auto&& tl : std::forward<Map>(t).terrain_layers)
			m.terrain_layers.emplace_back(to_raw_map(std::forward<std::remove_reference_t<decltype(tl)>>(tl)));

		if constexpr (std::is_rvalue_reference_v<decltype(t)>)
		{
			// terrain_layers and terrain_vertex are filled with moved from
			// containers at this point, so we clear to get rid of all the moved
			// from objects, so no-one can try and read them.
			t.terrain_layers.clear();
			t.terrain_vertex.clear();
		}

		m.width = t.width;

		return m;
	}

	raw_terrain_map to_raw_terrain_map(const terrain_map &t, const resources::terrain_settings& s)
	{
		return to_raw_terrain_map_impl(t, s);
	}

	raw_terrain_map to_raw_terrain_map(terrain_map&& t, const resources::terrain_settings& s)
	{
		return to_raw_terrain_map_impl(std::move(t), s);
	}

	terrain_map make_map(const tile_position size, const resources::terrainset *terrainset,
		const resources::terrain *t, const resources::terrain_settings& s)
	{
		assert(terrainset);
		assert(t);

		auto map = terrain_map{};
		map.terrainset = terrainset;
		const auto vertex_size = (integer_cast<std::size_t>(size.x) + 1) * (integer_cast<std::size_t>(size.y) + 1);
		constexpr auto max_size = terrain_map::max_size * terrain_map::max_size;
		if (std::cmp_greater_equal(vertex_size, max_size))
			throw terrain_error{ "Map exceeds maximum size" };

		map.width = size.x + 1;
        map.terrain_vertex.resize(vertex_size, t);
		const auto tile_count = integer_cast<std::size_t>(size.x) * integer_cast<std::size_t>(size.y);
		map.heightmap.resize(vertex_size, s.height_default);
		map.cliff_layer.resize(tile_count, s.cliff_default);
		map.ramp_layer.resize(tile_count);
		
		const auto& empty_tile = resources::get_empty_tile(s);
		const auto empty_layer = make_map(size, empty_tile, s);

		// empty cliff layer should just be empty tiles
		const auto empty_terrain = resources::get_empty_terrain(s);
		const auto empty_id = make_tile_id(map.cliff_tiles, empty_tile, empty_terrain);
		const auto cliff_width = tile_count * enum_type(terrain_map::cliff_layer_layout::multiplier);
		map.cliff_tiles.tiles.resize(cliff_width, empty_id);
		map.cliff_tiles.width = integer_cast<tile_index_t>(cliff_width);

		if (t != empty_terrain)
		{
			//fill in the correct terrain layer
			const auto index = get_terrain_id(terrainset, t);

			const auto end = std::size(terrainset->terrains);
			map.terrain_layers.reserve(std::size(terrainset->terrains));
			for (auto i = std::size_t{}; i < end; ++i)
			{
				if (i == index)
				{
					const auto& tiles = resources::get_transitions(*terrainset->terrains[i],
						resources::transition_tile_type::all, s);

					map.terrain_layers.emplace_back(make_map(
						size,
						tiles,
						s
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

		assert(is_valid(to_raw_terrain_map(map, s)));

		return map;
	}

	terrain_index_t get_width(const terrain_map &m) noexcept
	{
		return m.width;
	}

	tile_index_t get_tile_width(const terrain_map& m) noexcept
	{
		return m.width - 1;
	}

	tile_position get_size(const terrain_map& t)
	{
		if (t.width == 0 || t.terrain_vertex.empty())
			throw terrain_error{ "malformed terrain_map"s };

		return { 
			integer_cast<tile_position::value_type>(t.width - 1),
			integer_cast<tile_position::value_type>(integer_cast<tile_index_t>(t.terrain_vertex.size()) / t.width - 1)
		};
	}

	terrain_vertex_position get_vertex_size(const terrain_map& t)
	{
		return get_size(t) + tile_position{ 1, 1 };
	}

	// ramps must share a vertex or results will be undefined
	static std::optional<ramp_type> merge_ramps(ramp_type a, ramp_type b) noexcept
	{
		if (a == ramp_type::no_ramp)
			return b;

		if (b == ramp_type::no_ramp)
			return a;

		if (a == b)
			return a;
		return {};
	}

	namespace ramp
	{
		enum height_change : std::int8_t {
			lower_2 = -2,
			lower,
			none,
			raise,
			raise_2,
		};

		// defined below
		static std::array<height_change, 4> adjacent_ramp_shared_vertex(tile_position, const std::array<ramp_type, 4>&, const terrain_map&);
	}

	cell_height_data get_height_for_cell(tile_position p, const terrain_map& m, const resources::terrain_settings& s)
	{
		const auto tile_index = to_tile_index(p, m);
		assert(s.cliff_height > 0);

		const auto cliff_height = m.cliff_layer[tile_index] * s.cliff_height;
		const auto cliff_layer_half = s.cliff_height / 2;

		std::array<std::uint8_t, 4> corner_heights [[indeterminate]];

		// basic cliff height
		const auto index = to_vertex_index(p, m);
		corner_heights[enum_type(rect_corners::top_left)] = integer_clamp_cast<std::uint8_t>(m.heightmap[index] + cliff_height);
		corner_heights[enum_type(rect_corners::top_right)] = integer_clamp_cast<std::uint8_t>(m.heightmap[integer_cast<std::size_t>(index) + 1] + cliff_height);
		const auto index2 = index + m.width;
		corner_heights[enum_type(rect_corners::bottom_left)] = integer_clamp_cast<std::uint8_t>(m.heightmap[index2] + cliff_height);
		corner_heights[enum_type(rect_corners::bottom_right)] = integer_clamp_cast<std::uint8_t>(m.heightmap[integer_cast<std::size_t>(index2) + 1] + cliff_height);

		auto adjusted_corner_heights = corner_heights;
		const auto our_ramp_flags = std::bitset<4>{ m.ramp_layer[tile_index] };
		// adjust for our ramp type
		const auto ramps = get_adjacent_ramps(p, m);

		{
			// no ramps to our tile
			// time to check adjacents
			const auto adj_ramps = ramp::adjacent_ramp_shared_vertex(p, ramps, m);
			for (auto i = rect_corners::begin; i < rect_corners::end; i = next(i))
			{
				switch (adj_ramps[enum_type(i)])
				{
				case ramp::height_change::raise:
					adjusted_corner_heights[enum_type(i)] = integer_clamp_cast<std::uint8_t>(corner_heights[enum_type(i)] + cliff_layer_half);
					break;
				case ramp::height_change::lower:
					adjusted_corner_heights[enum_type(i)] = integer_clamp_cast<std::uint8_t>(corner_heights[enum_type(i)] - cliff_layer_half);
					break;
				case ramp::height_change::lower_2:
					adjusted_corner_heights[enum_type(i)] = integer_clamp_cast<std::uint8_t>(corner_heights[enum_type(i)] - s.cliff_height);
					break;
				case ramp::height_change::raise_2:
					adjusted_corner_heights[enum_type(i)] = integer_clamp_cast<std::uint8_t>(corner_heights[enum_type(i)] + s.cliff_height);
					break;
				default:
					break; // do nothing
				}
			}
		}

		// TOP_LEFT corner
		const auto& final_tl = adjusted_corner_heights[enum_type(rect_corners::top_left)];
		// TOP_RIGHT corner
		const auto& final_tr = adjusted_corner_heights[enum_type(rect_corners::top_right)];
		// BOTTOM_RIGHT corner
		const auto& final_br = adjusted_corner_heights[enum_type(rect_corners::bottom_right)];
		// BOTTOM_LEFT corner
		const auto& final_bl = adjusted_corner_heights[enum_type(rect_corners::bottom_left)];

		return cell_height_data{ final_tl, final_tr, final_br, final_bl };
	}

	bool within_map(terrain_vertex_position s, terrain_vertex_position p) noexcept
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
		const auto s = get_vertex_size(m);
		return within_map(s, p);
	}

	bool edge_of_map(const terrain_vertex_position map_size, const terrain_vertex_position position) noexcept
	{
		return position.x == 0 ||
			position.y == 0 ||
			position.x == map_size.x ||
			position.y == map_size.y;
	}

	namespace detail
	{
		// access a triangle pair range [0-5] using quad indexes[0-3] 
		// 'Func' is called to merge triangle elements
		// where multiple values are available for a quad corner
		template<invocable_r<const std::uint8_t&, std::uint8_t, std::uint8_t> Func>
		std::uint8_t read_triangles_as_quad(const triangle_height_data& t, rect_corners c, Func&& f)
			noexcept(std::is_nothrow_invocable_v<Func, std::uint8_t, std::uint8_t>)
		{
			const auto [first, second] = quad_index_to_triangle_index(c, t.triangle_type);
			if (second != bad_triangle_index && first != bad_triangle_index)
				return std::invoke(f, t.height[first], t.height[second]);
			else if (second != bad_triangle_index)
				return t.height[second];
			else
				return t.height[first];
		}
	}

	const resources::terrain *get_corner(const tile_corners &t, rect_corners c) noexcept
	{
		return t[enum_type(c)];
	}

	resources::transition_tile_type get_transition_type(const std::array<bool, 4u> &arr) noexcept
	{
		auto type = std::underlying_type_t<resources::transition_tile_type>{};

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

	resources::transition_tile_type get_transition_type(const std::bitset<4> b ) noexcept
	{
		auto type = std::underlying_type_t<resources::transition_tile_type>{};

		if (b.test(enum_type(rect_corners::top_left)))
			type += 8u;
		if (b.test(enum_type(rect_corners::top_right)))
			type += 1u;
		if (b.test(enum_type(rect_corners::bottom_right)))
			type += 2u;
		if (b.test(enum_type(rect_corners::bottom_left)))
			type += 4u;

		return resources::transition_tile_type{ type };
	}

	tile_corners get_terrain_at_tile(const terrain_map &m, tile_position p, const resources::terrain_settings& s)
	{
		const auto w = get_width(m);
		return get_terrain_at_tile(m.terrain_vertex, w, p, s);
	}

	terrain_map::vertex_height_t get_vertex_height(const terrain_vertex_position v, const terrain_map& m)
	{
		const auto index = to_vertex_index(v, m);
		assert(index < size(m.heightmap));
		return m.heightmap[index];
	}

	terrain_map::cliff_layer_t get_cliff_layer(const tile_position p, const terrain_map& m)
	{
		const auto index = to_tile_index(p, m);
		assert(index < size(m.cliff_layer));
		return m.cliff_layer[index];
	}

	namespace ramp
	{
		constexpr auto adj_tiles = std::array<tile_position, 4> {
			tile_position{ 0, -1 }, // top
			tile_position{ 1, 0 }, // right
			tile_position{ 0, 1 }, // bottom
			tile_position{ -1, 0 }, // left
		};

		constexpr auto reverse_edges = std::array<rect_edges, 4>{
			rect_edges::bottom,
			rect_edges::left,
			rect_edges::top,
			rect_edges::right,
		};

		static constexpr ramp_type to_ramp_type(std::uint8_t starting_cliff, std::uint8_t next_cliff) noexcept
		{
			const auto ramp_type_int = integer_cast<std::int8_t>(next_cliff - starting_cliff);
			switch (ramp_type_int)
			{
			case 1:
				[[fallthrough]];
			case -1:
				return ramp_type{ ramp_type_int };
			default:
				return ramp_type::no_ramp;
			}
		}

		constexpr auto diag_ramp_corner_checks = std::array<tile_position, 4>{
			tile_position{ -1, -1 }, // top left
			tile_position{  1, -1 }, // top right
			tile_position{  1,  1 }, // bottom rig
			tile_position{ -1,  1 }, // bottom left
		};

		// edges facing towards to the current tile
		constexpr auto our_ramp_edge_checks = std::array<std::tuple<rect_edges, rect_edges>, 4>{
			std::tuple{ rect_edges::left,	rect_edges::top		}, // top left
			std::tuple{ rect_edges::right,	rect_edges::top		}, // top right
			std::tuple{ rect_edges::right,	rect_edges::bottom	}, // bottom right
			std::tuple{ rect_edges::left,	rect_edges::bottom	}, // bottom left
		};

		// index return valure with rect_corners
		static std::array<height_change, 4> adjacent_ramp_shared_vertex(const tile_position p, const std::array<ramp_type, 4>& our_ramps, const terrain_map& m)
		{
			const auto world_size = get_size(m);
			const auto start_index = to_tile_index(p, world_size.x);
			const auto start_cliff = m.cliff_layer[start_index];
			constexpr auto max_diff = std::numeric_limits<int>::max();

			const auto check_ramp = [&](const tile_position offset, const rect_edges e)->std::tuple<ramp_type, int> {
				const auto pos = p + offset;
				if (!within_world(pos, world_size))
					return { ramp_type::no_ramp, max_diff };

				const auto index = to_tile_index(pos, world_size.x);
				const auto cliff = m.cliff_layer[index];
				
				const auto ramp = std::bitset<4>{ m.ramp_layer[index] };
				if (!ramp.test(enum_type(e)))
					return { ramp_type::no_ramp, max_diff };

				const auto next_pos = adj_tiles[enum_type(e)] + pos;
				// ramp flags must point towards tiles inside the world
				assert(within_world(next_pos, world_size));

				const auto diff = cliff - start_cliff;
				const auto next_index = to_tile_index(next_pos, world_size.x);
				const auto next_cliff = m.cliff_layer[next_index];
				return { to_ramp_type(cliff, next_cliff), diff };
			};

			constexpr auto apply_change = [](height_change& ch, const ramp_type e) noexcept {
				switch (e)
				{
				default:
					break;
				case ramp_type::uphill:
					ch = next(ch);
					break;
				case ramp_type::downhill:
					ch = prev(ch);
					break;
				}
				return;
			};

			std::array<height_change, 4> out [[indeterminate]];
			for (auto i = rect_corners::begin; i < rect_corners::end; i = next(i))
			{
				auto change = height_change::none;
				// the edges we need to check from our ramps
				const auto [our_edge1, our_edge2] = our_ramp_edge_checks[enum_type(i)];
				const auto our_ramp1 = our_ramps[enum_type(our_edge1)];
				const auto our_ramp2 = our_ramps[enum_type(our_edge2)];
				const auto our_merg = merge_ramps(our_ramp1, our_ramp2).value_or(ramp_type::no_ramp);
				apply_change(change, our_merg);

				const auto diag = diag_ramp_corner_checks[enum_type(i)];
				const auto other_edge1 = reverse_edges[enum_type(our_edge1)];
				const auto other_edge2 = reverse_edges[enum_type(our_edge2)];
				const auto [ramp1, diff1] = check_ramp(diag, other_edge1);
				const auto [ramp2, diff2] = check_ramp(diag, other_edge2);
				const auto& diff = diff1 == max_diff ? diff2 : diff1;

				const auto merg = merge_ramps(ramp1, ramp2).value_or(ramp_type::no_ramp);
				switch(change)
				{
				case -1:
					if (diff == 1)
						change = next(change);
					else if (diff == -1 && merg == our_merg)
						change = prev(change);
					break;
				case 0:
					if (diff == 0)
						apply_change(change, merg);
					else if (diff == 1 && merg == ramp_type::downhill)
						change = next(change);
					else if (diff == -1 && merg == ramp_type::uphill)
						change = prev(change);
					break;
				case 1:
					if (diff == 1 && merg == our_merg)
						change = next(change);
					else if (diff == -1)
						change = prev(change);
					break;
				}

				switch (change)
				{
				default:
					out[enum_type(i)] = height_change::none;
					break;
				case -2:
					out[enum_type(i)] = height_change::lower_2;
					break;
				case -1:
					out[enum_type(i)] = height_change::lower;
					break;
				case 1:
					out[enum_type(i)] = height_change::raise;
					break;
				case 2:
					out[enum_type(i)] = height_change::raise_2;
					break;
				}
			}

			return out;
		}
	}

	adjacent_cliffs get_adjacent_cliffs(const tile_position p, const terrain_map& m)
	{
		const auto world_size = get_size(m);
		const auto start_index = to_tile_index(p, m);
		const auto our_layer = m.cliff_layer[start_index];
		const auto our_ramp = std::bitset<4>{ m.ramp_layer[start_index] };
		
		// check edges
		auto out = adjacent_cliffs{};
		for (auto i = rect_edges::begin; i < rect_edges::end; i = next(i))
		{
			const auto pos = ramp::adj_tiles[enum_type(i)] + p;
			auto cliff = std::int8_t{};
			if (within_world(pos, world_size))
			{
				const auto index = to_tile_index(pos, m);
				const auto cliff_layer = m.cliff_layer[index];
				cliff = integer_cast<std::int8_t>(cliff_layer - our_layer);
			}

			// don't count ramps as cliffs
			if (our_ramp.test(enum_type(i)))
				cliff = {};

			out.set(enum_type(i), cliff > 0);
		}

		return out;
	}

	cliff_corners get_cliff_corners(const tile_position p, const terrain_map& m)
	{
		// these are the corners affected when checking each edge
		constexpr auto adj_tiles_corners = std::array<std::tuple<rect_corners, rect_corners>, 4>{
			std::tuple{ rect_corners::top_left,		rect_corners::top_right},
			std::tuple{ rect_corners::top_right,	rect_corners::bottom_right},
			std::tuple{ rect_corners::bottom_left,	rect_corners::bottom_right},
			std::tuple{ rect_corners::top_left,		rect_corners::bottom_left}
		};

		constexpr auto adj_tiles_diag = std::array<std::tuple<tile_position, tile_position>, 4>{
			std::tuple{ tile_position{ -1, -1 }, tile_position{  1, -1 } }, // top_left, top_right
			std::tuple{ tile_position{  1, -1 }, tile_position{  1,  1 } }, // top_right, bottom_right
			std::tuple{ tile_position{ -1,  1 }, tile_position{  1,  1 } }, // bottom_right, bottom_left
			std::tuple{ tile_position{ -1, -1 }, tile_position{ -1,  1 } }  // top_left, bottom_left
		};

		// ramp edges to check to each the diag tiles from the current adj tiles
		constexpr auto adj_tiles_ramp_edge_check = std::array<std::tuple<rect_edges, rect_edges>, 4>{
			std::tuple{ rect_edges::left,	rect_edges::right	},
			std::tuple{ rect_edges::top,	rect_edges::bottom	},
			std::tuple{ rect_edges::left,	rect_edges::right	},
			std::tuple{ rect_edges::top,	rect_edges::bottom	}
		};

		const auto world_size = get_size(m);
		const auto start_index = to_tile_index(p, m);
		const auto our_layer = m.cliff_layer[start_index];
		const auto our_ramp = std::bitset<4>{ m.ramp_layer[start_index] };
		
		auto out = cliff_corners{};
		for (auto i = rect_edges::begin; i < rect_edges::end; i = next(i))
		{
			const auto pos = ramp::adj_tiles[enum_type(i)] + p;
			if (within_world(pos, world_size))
			{
				const auto index = to_tile_index(pos, world_size.x);
				const auto cliff_layer = m.cliff_layer[index];
				const auto cliff = integer_cast<std::int8_t>(cliff_layer - our_layer);
				const auto [c1, c2] = adj_tiles_corners[enum_type(i)];
				if (cliff != 0 && !our_ramp.test(enum_type(i)))
				{
					out.set(enum_type(c1));
					out.set(enum_type(c2));
				}

				const auto [adj1, adj2] = adj_tiles_diag[enum_type(i)];
				const auto [edge1, edge2] = adj_tiles_ramp_edge_check[enum_type(i)];
				const auto adj_ramp = std::bitset<4>{ m.ramp_layer[index] };

				const auto adj1_pos = adj1 + p;
				if (within_world(adj1_pos, world_size))
				{
					const auto adj_index = to_tile_index(adj1_pos, world_size.x);
					const auto adj_cliff_layer = m.cliff_layer[adj_index];
					const auto adj_cliff = integer_cast<std::int8_t>(adj_cliff_layer - cliff_layer);
					if (adj_cliff != 0 && !adj_ramp.test(enum_type(edge1)))
						out.set(enum_type(c1));
				}

				const auto adj2_pos = adj2 + p;
				if (within_world(adj2_pos, world_size))
				{
					const auto adj_index = to_tile_index(adj2_pos, world_size.x);
					const auto adj_cliff_layer = m.cliff_layer[adj_index];
					const auto adj_cliff = integer_cast<std::int8_t>(adj_cliff_layer - cliff_layer);
					if (adj_cliff != 0 && !adj_ramp.test(enum_type(edge2)))
						out.set(enum_type(c2));
				}
			}
		}

		return out;
	}

	std::bitset<4> get_ramps(const tile_position p, const terrain_map& m)
	{
		const auto world_size = get_size(m);
		assert(within_world(p, world_size));
		const auto start_index = to_tile_index(p, world_size.x);
		return std::bitset<4>{ m.ramp_layer[start_index] };
	}

	std::array<ramp_type, 4> get_adjacent_ramps(const tile_position p, const terrain_map& m)
	{
		const auto world_size = get_size(m);
		assert(within_world(p, world_size));
		const auto start_index = to_tile_index(p, world_size.x);
		const auto our_ramp = std::bitset<4>{ m.ramp_layer[start_index] };

		auto ret = std::array<ramp_type, 4>{};
		static_assert(ramp_type{} == ramp_type::no_ramp, "Function expects default value in ret to be no_ramp");

		const auto cliff_layer = m.cliff_layer[start_index];
		for (auto i = rect_edges::begin; i < rect_edges::end; i = next(i))
		{
			const auto pos = ramp::adj_tiles[enum_type(i)] + p;
			if (within_world(pos, world_size))
			{
				const auto next_index = to_tile_index(pos, world_size.x);
				const auto next_ramp = std::bitset<4>{ m.ramp_layer[next_index] }.test(enum_type(ramp::reverse_edges[enum_type(i)]));
				const auto ramp_flags = our_ramp.test(enum_type(i)) && next_ramp;

				const auto next_cliff_layer = m.cliff_layer[next_index];
				const auto ramp_type = ramp::to_ramp_type(cliff_layer, next_cliff_layer);

				if (ramp_flags)
					ret[enum_type(i)] = ramp_type;
			}
		}

		return ret;
	}

	const resources::terrain *get_vertex(const terrain_map &m, const terrain_vertex_position p)
	{
		const auto index = to_1d_index(p, get_width(m));
        return m.terrain_vertex[integer_cast<std::size_t>(index)];
	}

	tag_list get_tags_at(const terrain_map &m, const tile_position p)
	{
		const auto settings = resources::get_terrain_settings();
		assert(settings);
		const auto corners = get_terrain_at_tile(m, p, *settings);

		tag_list out;
		for (const auto *t : corners)
		{
			assert(t);
			out.insert(std::end(out), std::begin(t->tags), std::end(t->tags));
		}

		// TODO: cliff tags?

		return out;
	}

	void resize_map_relative(terrain_map& m, const vector2_int top_left, const vector2_int bottom_right,
		const resources::terrain* t, const std::uint8_t height, const resources::terrain_settings& s)
	{
		const auto [current_height, current_width] = get_size(m);
        
        const auto new_height = current_height - top_left.y + bottom_right.y;
        const auto new_width = current_width - top_left.x + bottom_right.x;

		const auto size = vector2_int{
            new_width,
            new_height
		};

		resize_map(m, size, { -top_left.x, -top_left.y }, t, height, s);
	}

	void resize_map(terrain_map& m, const vector2_int s, const vector2_int o,
		const resources::terrain* t, const std::uint8_t height, const resources::terrain_settings& settings)
	{
		const auto old_size = get_vertex_size(m);
		
		//update terrain vertex
		auto new_terrain = table_reduce_view{ vector2_int{},
			s + vector2_int{ 1, 1 }, t, [](auto&&, auto&& t) noexcept -> const resources::terrain* {
				return t;
		} };

		const auto current_terrain = table_view{ o, old_size, m.terrain_vertex };

		new_terrain.add_table(current_terrain);
		
		auto vertex = std::vector<const resources::terrain*>{};
		const auto size = (s.x + 1) * (s.y + 1);
		vertex.reserve(size);

		for (auto i = vector2_int::value_type{}; i < size; ++i)
			vertex.emplace_back(new_terrain[i]);

		m.terrain_vertex = std::move(vertex);

		//regenerate terrain layers
		m.terrain_layers = generate_terrain_layers(m.terrainset,
			m.terrain_vertex, integer_cast<terrain_index_t>(s.x), settings);

		//TODO: update with new heightmap system
		static_assert(std::same_as<decltype(m.heightmap)::value_type, std::uint8_t>);
		
		// expand heightmap
		const auto current_height = table_view{ o, old_size, m.heightmap };
		auto new_height = table_reduce_view{ vector2_int{},
			s + vector2_int{ 1, 1 }, height, [](auto&&, auto&& h) noexcept -> std::uint8_t {
				return h;
		} };
		new_height.add_table(current_height);

		auto height_table = std::vector<std::uint8_t>{};
		height_table.reserve(size);

		for (auto i = vector2_int::value_type{}; i < size; ++i)
			height_table.emplace_back(new_height[i]);

		m.heightmap = std::move(height_table);

		return;
	}

	// returns each cell adjacent to this vertex
	static constexpr auto get_adjacent_tiles(const terrain_vertex_position p) noexcept
	{
		return std::array{
			tile_position{ p.x - 1, p.y -1	},	// Top left
			tile_position{ p.x,		p.y -1	},	// Top right
			tile_position{ p.x - 1, p.y		},	// Bottom left
			tile_position{ p }					// Bottom right
		};
	}

	static void place_terrain_internal(terrain_map &m, const terrain_vertex_position p,
		const resources::terrain *t)
	{
		assert(t);
		const auto index = to_1d_index(p, get_width(m));
        m.terrain_vertex[integer_cast<std::size_t>(index)] = t;
	}

	static void update_tile_layers(terrain_map &m, terrain_vertex_position pos, const resources::terrain_settings& s)
	{
		const auto t_p = get_adjacent_tiles(pos);

		const auto begin = std::cbegin(m.terrainset->terrains);
		const auto end = std::cend(m.terrainset->terrains);
		auto terrain_iter = std::begin(m.terrainset->terrains);
		auto layer_iter = std::begin(m.terrain_layers);
		assert(std::size(m.terrain_layers) == std::size(m.terrainset->terrains));
		for (/*terrain_iter, layer_iter*/; terrain_iter != end; ++terrain_iter, ++layer_iter)
		{
			for (const auto& p : t_p)
			{
				//get the terrain corners for this tile
				const auto corners = get_terrain_at_tile(m, p, s);

				const auto equal_terrain = [t = *terrain_iter](auto&& other_t) {
					return t.get() == other_t;
					};

				//get the transition, only looking for terrains above the current one
				const auto transition = [&] {
					if (std::none_of(std::begin(corners), std::end(corners), equal_terrain))
						return resources::transition_tile_type::none;
					else
						return get_transition_type(corners, begin, std::next(terrain_iter));
					}();

				const auto tile = resources::get_random_tile(**terrain_iter, transition, s);
				place_tile(*layer_iter, p, tile, s);
			}
		}
	}

	void place_terrain(terrain_map &m, terrain_vertex_position p, const resources::terrain *t,
		const resources::terrain_settings& s)
	{
		if (!t)
			throw terrain_error{ "place_terrain: terrain parameter was null"s };
		else if (within_map(m, p))
		{
			place_terrain_internal(m, p, t);
			update_tile_layers(m, p, s);
		}
	}

	// returns true if a ramp can be added to this edge(accounting for compatibility
	//	with already existing ramps, uses rect_edges e to disabiguate
	static bool can_ramp_one_edge(const rect_edges e, const tile_position p, const terrain_map& m) noexcept
	{
		const auto w_size = get_size(m);
		const auto i = to_tile_index(p, w_size.x);
		const auto start_cliff = m.cliff_layer[i];

		const auto get_ramp = [&](const rect_edges edge)->std::tuple<ramp_type, bool> {
			const auto tile = p + ramp::adj_tiles[enum_type(edge)];
			if (!within_world(tile, w_size))
				return { ramp_type::no_ramp, false };
			const auto index = to_tile_index(tile, w_size.x);
			const auto cliff = m.cliff_layer[index];
			const auto ramp = std::bitset<4>{ m.ramp_layer[index] }.test(enum_type(ramp::reverse_edges[enum_type(edge)]));
			return { ramp::to_ramp_type(start_cliff, cliff), ramp };
		};

		// a and b must be compatible for the same ramp as e
		const auto [type_e, ramp_e]  = get_ramp(e);
		if (type_e == ramp_type::no_ramp)
			return false;

		const auto [a, b] = [](const rect_edges edge) noexcept -> std::tuple<rect_edges, rect_edges> {
			using enum rect_edges;
			switch (edge)
			{
			default:
				[[fallthrough]];
			case bottom:
				[[fallthrough]];
			case top:
				return { left, right };
			case left:
				[[fallthrough]];
			case right:
				return { top, bottom };
			}
		}(e);

		const auto [type_a, ramp_a] = get_ramp(a);
		const auto [type_b, ramp_b] = get_ramp(b);
		const auto merg_a = !ramp_a || merge_ramps(type_e, type_a);
		const auto merg_b = !ramp_b || merge_ramps(type_e, type_b);
		return merg_a == merg_b;
	}

	// returns true if a ramp can be added to this position
	// a ramp can only be added if it would be unambigous 
	static bool can_ramp(const rect_edges e, const tile_position p, const terrain_map& m) noexcept
	{
		const auto w_size = get_size(m);
		const auto i = to_tile_index(p, w_size.x);
		const auto start_cliff = m.cliff_layer[i];

		const auto get_ramp = [&](const rect_edges edge) {
			const auto tile = p + ramp::adj_tiles[enum_type(edge)];
			if (!within_world(tile, w_size))
				return ramp_type::no_ramp;
			const auto index = to_tile_index(tile, w_size.x);
			const auto cliff = m.cliff_layer[index];
			return ramp::to_ramp_type(start_cliff, cliff);
			};

		// a and b must be compatible for the same ramp as e
		const auto ramp_e = get_ramp(e);
		if (ramp_e == ramp_type::no_ramp)
			return false;

		// TODO: this might need to be pulled out for reuse
		const auto [a, b] = [](const rect_edges edge) noexcept -> std::tuple<rect_edges, rect_edges> {
			using enum rect_edges;
			switch (edge)
			{
			default:
				[[fallthrough]];
			case bottom:
				[[fallthrough]];
			case top:
				return { left, right };
			case left:
				[[fallthrough]];
			case right:
				return { top, bottom };
			}
			}(e);

		const auto ramp_a = get_ramp(a), ramp_b = get_ramp(b);
		const auto merg_a = merge_ramps(ramp_e, ramp_a), merg_b = merge_ramps(ramp_e, ramp_b);
		
		return merg_a == merg_b;
	}

	// we can only add a ramp on the tile if the two ramps are on opposite sides of the tile
	//	or the adjacent tiles are both uphill or both downhill
	std::bitset<4> can_add_ramp(const tile_position p, const terrain_map& m)
	{
		const auto world_size = get_size(m);

		auto ret = std::bitset<4>{};
		for (auto i = rect_edges::begin; i < rect_edges::end; i = next(i))
		{
			if (can_ramp(i, p, m) && !can_ramp(ramp::reverse_edges[enum_type(i)], p, m)) // we can only ramp if it's valid to ramp from both sides
				ret.set(enum_type(i), can_ramp_one_edge(ramp::reverse_edges[enum_type(i)], ramp::adj_tiles[enum_type(i)] + p, m));
		}

		return ret;		
	}

	static void set_ramp_flag(std::span<terrain_map::ramp_layer_t> ramp_layer, const tile_index_t index, const rect_edges edge)
	{
		assert(index < size(ramp_layer));
		auto& ramp = ramp_layer[index];
		auto bset = std::bitset<4>{ ramp };
		bset.set(enum_type(edge));
		ramp = integer_cast<terrain_map::ramp_layer_t>(bset.to_ulong());
		return;
	}

	bool place_ramp(const tile_position p, terrain_map& m, const resources::terrain_settings& s)
	{
		const auto ramp = can_add_ramp(p, m);
		const auto world_size = get_size(m);
		const auto starting_index = to_tile_index(p, world_size.x);
		auto changed = false;
		for (auto i = rect_edges::begin; i < rect_edges::end; i = next(i))
		{
			if (ramp.test(enum_type(i)))
			{
				set_ramp_flag(m.ramp_layer, starting_index, i);
				const auto other_pos = ramp::adj_tiles[enum_type(i)] + p;
				const auto other_index = to_tile_index(other_pos, world_size.x);
				set_ramp_flag(m.ramp_layer, other_index, ramp::reverse_edges[enum_type(i)]);
				changed = true;
			}
		}

		if(changed)
			detail::regenerate_nearby_cliff_tiles(p, m, s);
		return changed;
	}

	bool clear_ramp(const tile_position p, terrain_map& m, const resources::terrain_settings& s)
	{
		const auto world_size = get_size(m);
		assert(within_world(p, world_size));
		const auto start_index = to_tile_index(p, world_size.x);
		auto& ramp = m.ramp_layer[start_index];
		auto bset = std::bitset<4>{ ramp };
		auto changed = false;
		// clear any adjacent ramp tiles that were connected to this one
		for (auto i = rect_edges::begin; i < rect_edges::end; i = next(i))
		{
			if (bset.test(enum_type(i)))
			{
				const auto other_pos = ramp::adj_tiles[enum_type(i)] + p;
				const auto other_index = to_tile_index(other_pos, world_size.x);
				auto& other_ramp = m.ramp_layer[other_index];
				auto other_bset = std::bitset<4>{ other_ramp };
				other_bset.reset(enum_type(ramp::reverse_edges[enum_type(i)]));
				other_ramp = integer_cast<terrain_map::ramp_layer_t>(other_bset.to_ulong());
				changed = true;
			}
		}

		bset.reset();
		ramp = integer_cast<terrain_map::ramp_layer_t>(bset.to_ulong());

		if(changed)
			detail::regenerate_nearby_cliff_tiles(p, m, s);
		return changed;
	}

	namespace
	{
		constexpr vector2_float make_quad_point(const tile_position pos, const float tile_sizef,
			const rect_corners c) noexcept
		{
			auto ret = static_cast<vector2_float>(pos) * tile_sizef;

			switch (c)
			{
			case rect_corners::top_right:
				ret.x += tile_sizef;
				break;
			case rect_corners::bottom_right:
				ret.x += tile_sizef;
				[[fallthrough]];
			case rect_corners::bottom_left:
				ret.y += tile_sizef;
				break;
			}
			return ret;
		}

		constexpr std::array<vector2_float, 3> add_point_height(std::array<vector2_float, 3> pos,
			const vector2_float height_dir, const std::span<const std::uint8_t, 3> heightmap) noexcept
		{
			pos[0] += height_dir * float_cast(heightmap[0]);
			pos[1] += height_dir * float_cast(heightmap[1]);
			pos[2] += height_dir * float_cast(heightmap[2]);
			return pos;
		}

		struct tris
		{
			std::array<vector2_float, 3> left_tri, right_tri;
			std::array<vector2_float, 3> flat_left_tri, flat_right_tri;
		};

		constexpr tris get_quad_triangles(const tile_position pos, const float tile_sizef,
			const vector2_float height_dir, const cell_height_data& heightmap) noexcept
		{
			tris out [[indeterminate]];
			std::array<std::uint8_t, 6> tri_height [[indeterminate]];
			const auto& tl = heightmap[enum_type(rect_corners::top_left)];
			const auto& tr = heightmap[enum_type(rect_corners::top_right)];
			const auto& br = heightmap[enum_type(rect_corners::bottom_right)];
			const auto& bl = heightmap[enum_type(rect_corners::bottom_left)];

			if (pick_triangle_type(pos) == terrain_map::triangle_type::triangle_uphill)
			{
				out.flat_left_tri = { make_quad_point(pos, tile_sizef, rect_corners::top_left),
					make_quad_point(pos, tile_sizef, rect_corners::bottom_left),
					make_quad_point(pos, tile_sizef, rect_corners::top_right) };
				out.flat_right_tri = { out.flat_left_tri[2], out.flat_left_tri[1],
					make_quad_point(pos, tile_sizef, rect_corners::bottom_right) };

				tri_height = {
					tl, bl, tr,
					tr, bl, br
				};
			}
			else
			{
				out.flat_left_tri = { make_quad_point(pos, tile_sizef, rect_corners::top_left),
					make_quad_point(pos, tile_sizef, rect_corners::bottom_left),
					make_quad_point(pos, tile_sizef, rect_corners::bottom_right) };
				out.flat_right_tri = { out.flat_left_tri[0], out.flat_left_tri[2],
					make_quad_point(pos, tile_sizef, rect_corners::top_right) };

				tri_height = {
					tl, bl, br,
					tl, br, tr
				};
			}

			const auto begin = std::begin(tri_height);
			out.left_tri = add_point_height(out.flat_left_tri, height_dir,
				std::span<const std::uint8_t, 3>{ begin, 3u });
			out.right_tri = add_point_height(out.flat_right_tri, height_dir,
				std::span<const std::uint8_t, 3>{ std::next(begin, 3), 3u });
			return out;
		}

		// returns true if left is lower than right
		static bool lowest_tri(const std::array<vector2_float, 3>& left,
			const std::array<vector2_float, 3>& right, float rot) noexcept
		{
			auto lowest = std::numeric_limits<float>::max();
			auto left_point = false;
			auto current_point = true;
			const auto cos_theta = std::cos(rot);
			const auto sin_theta = std::sin(rot);
			const auto check_lowest = [&](const vector2_float &p) {
				const auto y = p.y * cos_theta + p.x * sin_theta;
				if (y < lowest)
				{
					lowest = y;
					left_point = current_point;
				}
				return;
				};

			std::ranges::for_each(left, check_lowest);
			current_point = false;
			std::ranges::for_each(right, check_lowest);
			return left_point;
		}
	}

	static constexpr std::tuple<rect_edges, rect_edges> visible_cliffs(float rot) noexcept
	{
		while (rot < 0.f)
			rot += 360.f;

		while (rot > 360.f)
			rot -= 360.f;

		if (rot <= 90.f)
			return std::tuple{ rect_edges::top, rect_edges::left };
		else if (rot <= 180.f)
			return std::tuple{ rect_edges::bottom, rect_edges::left };
		else if (rot <= 270.f)
			return std::tuple{ rect_edges::right, rect_edges::bottom };
		else
			return std::tuple{ rect_edges::top, rect_edges::right };
	}

	static const std::optional<tris> make_cliff_face(const rect_edges edge, const tile_position pos,
		const adjacent_cliffs adj_cliffs, const world_vector_t height_dir,
		const float tile_sizef, const terrain_map& m, const resources::terrain_settings& settings)
	{
		const auto adj_tile = ramp::adj_tiles[enum_type(edge)] + pos;
		const auto w_size = get_size(m);
		if (!within_world(adj_tile, w_size))
			return {};

		if (!adj_cliffs.test(enum_type(edge)))
			return {};

		tris out [[indeterminate]];

		switch (edge)
		{
		default:
			[[fallthrough]];
		case rect_edges::top:
			out.flat_left_tri = { make_quad_point(pos, tile_sizef, rect_corners::top_left),
					make_quad_point(pos, tile_sizef, rect_corners::top_left),
					make_quad_point(pos, tile_sizef, rect_corners::top_right) };
			break;
		case rect_edges::right:
			out.flat_left_tri = { make_quad_point(pos, tile_sizef, rect_corners::top_right),
					make_quad_point(pos, tile_sizef, rect_corners::top_right),
					make_quad_point(pos, tile_sizef, rect_corners::bottom_right) };
			break;
		case rect_edges::bottom:
			out.flat_left_tri = { make_quad_point(pos, tile_sizef, rect_corners::bottom_right),
					make_quad_point(pos, tile_sizef, rect_corners::bottom_right),
					make_quad_point(pos, tile_sizef, rect_corners::bottom_left) };
			break;

		case rect_edges::left:
			out.flat_left_tri = { make_quad_point(pos, tile_sizef, rect_corners::bottom_left),
					make_quad_point(pos, tile_sizef, rect_corners::bottom_left),
					make_quad_point(pos, tile_sizef, rect_corners::top_left) };
			break;
		}

		out.flat_right_tri = { out.flat_left_tri[2], out.flat_left_tri[0],
				out.flat_left_tri[2] };

		const auto bottom_height = get_height_for_cell(pos, m, settings);
		const auto top_height = get_height_for_cell(adj_tile, m, settings);

		std::array<std::uint8_t, 2> cliff_top [[indeterminate]],
			cliff_bottom [[indeterminate]];

		switch (edge)
		{
		default:
			[[fallthrough]];
		case rect_edges::top:
			cliff_top = get_height_for_bottom_edge(top_height);
			cliff_bottom = get_height_for_top_edge(bottom_height);
			break;
		case rect_edges::right:
			cliff_top = get_height_for_left_edge(top_height);
			cliff_bottom = get_height_for_right_edge(bottom_height);
			break;
		case rect_edges::bottom:
			cliff_top = get_height_for_top_edge(top_height);
			cliff_bottom = get_height_for_bottom_edge(bottom_height);
			break;
		case rect_edges::left:
			cliff_top = get_height_for_right_edge(top_height);
			cliff_bottom = get_height_for_left_edge(bottom_height);
			break;
		}
		
		out.left_tri[0] = out.flat_left_tri[0] + height_dir * float_cast(cliff_top[0]);
		out.left_tri[1] = out.flat_left_tri[1] + height_dir * float_cast(cliff_bottom[0]);
		out.left_tri[2] = out.flat_left_tri[2] + height_dir * float_cast(cliff_top[1]);

		out.right_tri[0] = out.flat_right_tri[0] + height_dir * float_cast(cliff_top[0]);
		out.right_tri[1] = out.flat_right_tri[1] + height_dir * float_cast(cliff_bottom[0]);
		out.right_tri[2] = out.flat_right_tri[2] + height_dir * float_cast(cliff_bottom[1]);

		return out;
	}

	static const std::tuple<std::optional<tris>, std::optional<tris>, rect_edges, rect_edges> make_cliff_faces(const tile_position tile_check,
		const float rot, const world_vector_t height_dir, const float tile_sizef,
		const terrain_map& m, const resources::terrain_settings& settings)
	{
		const auto [cliff1, cliff2] = visible_cliffs(rot);
		const auto adj_cliffs = get_adjacent_cliffs(tile_check, m);
		return { make_cliff_face(cliff1, tile_check, adj_cliffs, height_dir, tile_sizef, m, settings),
			make_cliff_face(cliff2, tile_check, adj_cliffs, height_dir, tile_sizef, m, settings),
			cliff1, cliff2 };
	}

	static std::optional<std::tuple<barycentric_point, std::array<vector2_float, 3>>> 
		check_quad(const world_vector_t p, const float rot, const tris quad_triangles) noexcept
	{
		// test for hit
		// NOTE: be mindful of how quads are triangled in terrain_map/animation.hpp
		// NOTE: this is the canonical vertex order for triangles in the terrain system
		auto left_hit = is_within(p, quad_triangles.left_tri),
			right_hit = is_within(p, quad_triangles.right_tri);

		// If we hit both tris in a quad, prioritise the tri with the closest point to the camera.
		if (left_hit && right_hit)
		{
			if (lowest_tri(quad_triangles.flat_left_tri, quad_triangles.flat_right_tri, rot))
				right_hit = false;
			else
				left_hit = false;
		}

		if (left_hit)
		{
			const auto bary_point = to_barycentric(p, quad_triangles.left_tri);
			const auto tri = quad_triangles.flat_left_tri;
			return std::tuple{ bary_point, tri };
		}
		else if (right_hit)
		{
			const auto bary_point = to_barycentric(p, quad_triangles.right_tri);
			const auto tri = quad_triangles.flat_right_tri;
			return std::tuple{ bary_point, tri };
		}

		return {};
	}

	// project 'p' onto the flat version of 'map'
	std::optional<terrain_target> project_onto_terrain(const world_vector_t p, const float rot,
		const terrain_map& map,	const resources::terrain_settings& settings)
	{
		const auto tile_size = settings.tile_size;
		const auto tile_sizef = float_cast(tile_size);
		const auto norm_p = p / tile_sizef;

		// walk over tiles using DDA as shown here: https://www.youtube.com/watch?v=NbSee-XM7WA
		// and here: https://lodev.org/cgtutor/raycasting.html

		constexpr auto rotation_offset = 270.f;
		auto advance_dir = to_vector(pol_vector2_t<float>{ to_radians(rot + rotation_offset), 1.f });
		advance_dir.y *= -1.f;
		const auto height_dir = advance_dir * -1.f;

		auto tile_check = tile_position{
			integral_cast<tile_position::value_type>(norm_p.x, round_towards_zero_tag),
			integral_cast<tile_position::value_type>(norm_p.y, round_towards_zero_tag)
		};

		const auto step = vector2_float{
			abs(1.f / advance_dir.x),
			abs(1.f / advance_dir.y)
		};

		auto dist = vector2_float{
			advance_dir.x < 0.f ? (norm_p.x - tile_check.x) * step.x : (tile_check.x + 1.f - norm_p.x) * step.x,
			advance_dir.y < 0.f ? (norm_p.y - tile_check.y) * step.y : (tile_check.y + 1.f - norm_p.y) * step.y
		};

		const auto tile_step = tile_position{
			advance_dir.x < 0.f ? -1 : 1,
			advance_dir.y < 0.f ? -1 : 1
		};

		// quad vertex for the last hit quad
		auto bary_point = barycentric_point{};
		auto last_tri = std::array<vector2_float, 3>{};
		auto tile_hit = tile_position{};
		auto cliff_target = rect_edges::end;
		
		// go from 'p' in the direction of advance dir, checking each tile to see if it is under the mouse
		// end after reaching the edge of the map
		const auto world_size = get_size(map);

		// we may have clicked above the map, but under high terrain, 
		// so run the algo until we reach the end of the world
		// if the loop go's on for too long then we're probably too far away or
		// coming from the wrong angle_theta, so bail if sentinel gets to large.
		{
			const auto limit = (255 / tile_size) + 1;
			auto sentinel = unsigned{};
			while (!within_world(tile_check, world_size) && std::cmp_less(sentinel++, limit))
			{
				if (dist.x < dist.y)
				{
					dist.x += step.x;
					tile_check.x += tile_step.x;
				}
				else
				{
					dist.y += step.y;
					tile_check.y += tile_step.y;
				}
			}
		}

		while (within_world(tile_check, world_size))
		{	
			// get index for accessing heightmap
			const auto height_info = get_height_for_cell(tile_check, map, settings);
			// generate quad vertex
			const auto quad_triangles = get_quad_triangles(tile_check, tile_sizef, height_dir, height_info);

			const auto quad_hit = check_quad(p, rot, quad_triangles);
			if (quad_hit)
			{
				std::tie(bary_point, last_tri) = *quad_hit;
				tile_hit = tile_check;
			}
			else
			{
				const auto [cliff1, cliff2, edge1, edge2] = make_cliff_faces(tile_check, rot, height_dir, tile_sizef, map, settings);
				if (cliff1)
				{
					const auto cliff_hit = check_quad(p, rot, *cliff1);
					if (cliff_hit)
					{
						std::tie(bary_point, last_tri) = *cliff_hit;
						tile_hit = tile_check;
						cliff_target = edge1;
					}
				}

				if (cliff2)
				{
					const auto cliff_hit = check_quad(p, rot, *cliff2);
					if (cliff_hit)
					{
						std::tie(bary_point, last_tri) = *cliff_hit;
						tile_hit = tile_check;
						cliff_target = edge2;
					}
				}
			}

			// update for next tile
			if (dist.x < dist.y)
			{
				dist.x += step.x;
				tile_check.x += tile_step.x;
			}
			else
			{
				dist.y += step.y;
				tile_check.y += tile_step.y;
			}
		}

		// mouse not over the terrain
		if (bary_point == barycentric_point{})
			return {};
		// calculate pos within the hit tri
		const auto pixel = from_barycentric(bary_point, last_tri);
		return terrain_target{ pixel, tile_hit, cliff_target };
	}
}

namespace hades::resources
{
	static void load_terrain(terrain& terr, data::data_manager &d)
	{
		apply_to_terrain(terr, [&d](std::vector<tile> &v){
			for (auto &t : v)
			{
				if (t.tex)
				{
					auto texture = d.get_resource(t.tex.id());
					if (!texture->loaded)
						texture->load(d);
				}
			}
		});

		terr.loaded = true;
	}

	void terrain::load(data::data_manager& d)
	{
		load_terrain(*this, d);
		return;
	}

	//short transition names, same values and order as the previous list
	// this is the format output by the terrain serialise func
	constexpr auto transition_short_names = std::array{
		"n"sv, // causes a tile skip
		"tr"sv,
		"br"sv,
		"tr-br"sv,
		"bl"sv,
		"tr-bl"sv,
		"blr"sv,
		"tr-blr"sv,
		"tl"sv,
		"tlr"sv,
		"tl-br"sv,
		"tlr-br"sv,
		"tl-bl"sv,
		"tlr-bl"sv,
		"tl-blr"sv,
		"a"sv
	};

	static_assert(std::signed_integral<tile_index_t>);
	constexpr auto tile_count_auto_val = tile_index_t{ -1 };

	void terrain::serialise(const data::data_manager& d, data::writer& w) const
	{
		w.start_map(d.get_as_string(id));
		serialise_impl(d, w); // tileset

		const auto prev = d.try_get_previous(this);

		auto beg = begin(terrain_source_groups);

		if (prev.result)
			advance(beg, size(prev.result->terrain_source_groups));

		if (beg != end(terrain_source_groups))
		{
			w.start_sequence("terrain"sv);
			std::for_each(beg, end(terrain_source_groups), [&](auto g) {
				w.start_map();
				w.write("texture"sv, g.texture);

				if (g.top != texture_size_t{})
					w.write("top"sv, g.top);
				if (g.left != texture_size_t{})
					w.write("left"sv, g.left);

				w.write("tiles-per-row"sv, g.tiles_per_row);

				if (g.tile_count != tile_count_auto_val) 
					w.write("tile-count"sv, g.tile_count);

				std::visit([&](const auto& layout) {
					using T = std::decay_t<decltype(layout)>;
					if constexpr (std::same_as<T, terrain_source::layout_type>)
						w.write("layout"sv, "default"sv);
					else
					{
						w.write("layout"sv);
						w.start_sequence();
						for (auto index : layout)
						{
							const auto i = integer_cast<std::size_t>(enum_type(index));
							assert(i < size(transition_short_names));
							w.write(transition_short_names[i]);
						}
						w.end_sequence();
					}
					}, g.layout);

				w.end_map();
				});

			w.end_sequence();
		}

		w.end_map();
		return;
	}

	void cliff_style::load(data::data_manager& d)
	{
		d.get<terrain>(cliff_surface_terrain->id);
		d.get<terrain>(cliff_face_terrain->id);
		loaded = true;
		return;
	}

	constexpr auto cliff_surface_terrain_str = "surface-terrain"sv;
	constexpr auto cliff_face_terrain_str = "face-terrain"sv;

	void cliff_style::serialise(const data::data_manager& d, data::writer& w) const
	{
		//cliff-styles:
		//	name: 
		//		surface-terrain: terrain
		//		face-terrain: terrain

		w.start_map(d.get_as_string(id));
		w.write(cliff_face_terrain_str, cliff_face_terrain.id());
		w.write(cliff_surface_terrain_str, cliff_surface_terrain.id());
		w.end_map();
		return;
	}

	void terrainset::load(data::data_manager& d)
	{
		for (const auto& t : terrains)
			d.get<terrain>(t->id);

		if (cliff_type)
			d.get<cliff_style>(cliff_type.id());

		loaded = true;
		return;
	}

	void terrainset::serialise(const data::data_manager& d, data::writer& w) const
	{
		//terrainsets:
		//	name: [terrains, terrains, terrains] // not a mergable sequence, this will overwrite

		w.write(d.get_as_string(id));
		w.start_sequence();
		for (auto& terr : terrains)
			w.write(terr.id());
		w.end_sequence();
		return;
	}

	static void load_terrain_settings(terrain_settings& s, data::data_manager &d)
	{
		detail::load_tile_settings(s, d);

		//load terrains
		if (s.empty_terrain)
			d.get<terrain>(s.empty_terrain.id());

		for (const auto& t : s.terrains)
			d.get<terrain>(t.id());

		for (const auto& t : s.terrainsets)
			d.get<terrainset>(t.id());

		if (s.cliff_height == 0)
			throw terrain_error{ "terrain-settings::cliff_height cannot be 0" };
		else if (std::cmp_greater_equal(s.cliff_max * s.cliff_height + s.height_max, 255))
			log_warning("Terrain Settings: height_max and cliff_max * cliff_height are too high; they should be less that 255 or the terrain renderer will overflow"sv);

		s.loaded = true;
		return;
	}

	void terrain_settings::load(data::data_manager& d) 
	{
		load_terrain_settings(*this, d);
		return;
	}

	void terrain_settings::serialise(const data::data_manager& d, data::writer& w) const
	{
		tile_settings::serialise(d, w);

		// TODO: other settings terrains
		if (editor_terrain)
			w.write("editor-terrain"sv, d.get_as_string(editor_terrain.id()));


		if(height_min != 1)
			w.write("height-min"sv, height_min);
		if(height_max != 150)
			w.write("height-max"sv, height_max);
		if (height_default != 100)
			w.write("height-default"sv, height_default);

		if (cliff_default != 5)
			w.write("cliff-default"sv, cliff_default);
		if (cliff_min != 1)
			w.write("cliff-min"sv, cliff_min);
		if (cliff_max != 10)
			w.write("cliff-max"sv, cliff_max);
		if (cliff_height != 10)
			w.write("cliff-height"sv, cliff_height);

		return;
	}

	// if anyone asks for the 'none' type just give them the empty tile
	template<class Terrain>
		requires std::same_as<resources::terrain, std::decay_t<Terrain>>
	auto get_transitions_impl(transition_tile_type type, Terrain& t, const resources::terrain_settings& s) 
		-> std::conditional_t<std::is_const_v<Terrain>, const std::vector<tile>&, std::vector<tile>&>
	{
		// is_const is only false for the functions inside this file
		// where we want to assert
		if constexpr (std::is_const_v<Terrain>)
		{
			if (type == transition_tile_type::none)
				return get_empty_terrain(s)->tiles;
		}
		else
			assert(type != transition_tile_type::none);

		//we store the 'all' tiles in the 'none' index
		//since it is unused
		if (type == transition_tile_type::all)
			type = transition_tile_type::none;
		assert(enum_type(type) < std::size(t.terrain_transition_tiles));
		return t.terrain_transition_tiles[enum_type(type)];
	}

	static void add_tiles_to_terrain(terrain &terrain, const vector2_int start_pos, resources::resource_link<resources::texture> tex,
		const std::vector<transition_tile_type> &tiles, const tile_index_t tiles_per_row, const resources::terrain_settings& s)
	{
		const auto& tile_size = s.tile_size;
		auto count = tile_size_t{};
		for (const auto t : tiles)
		{
			assert(t <= transition_tile_type::transition_end);

			const auto tile_x = (count % tiles_per_row) * tile_size + start_pos.x;
			const auto tile_y = (count / tiles_per_row) * tile_size + start_pos.y;

			++count;

			if (t == transition_tile_type::none)
			{
				LOGWARNING("skipping terrain tile, layouts cannot contain the 'none' transition");
				continue;
			}

			auto &transition_vector = get_transitions_impl(t, terrain, s);

			const auto tile = resources::tile{ tex, integer_cast<tile_size_t>(tile_x), integer_cast<tile_size_t>(tile_y), &terrain };
			transition_vector.emplace_back(tile);
			terrain.tiles.emplace_back(tile);
		}
	}

	static std::pair<std::vector<transition_tile_type>, terrain::terrain_source::layout_type>
		parse_layout(const std::vector<string> &s)
	{
		using layout_type = terrain::terrain_source::layout_type;

		//layouts
		//This is the layout used by warcraft 3 tilesets, a common layout
		// on tileset websites
		using t = transition_tile_type;
		constexpr auto war3_layout = std::array{ t::all, t::bottom_right, t::bottom_left, t::bottom_left_right,
				t::top_right, t::top_right_bottom_right, t::top_right_bottom_left, t::top_right_bottom_left_right,
				t::top_left, t::top_left_bottom_right, t::top_left_bottom_left, t::top_left_bottom_left_right,
				t::top_left_right, t::top_left_right_bottom_right, t::top_left_right_bottom_left, t::all };

		//default to the war 3 layout
		if (s.empty())
			return { { std::begin(war3_layout), std::end(war3_layout) }, layout_type::empty };

		using namespace std::string_literals;
		using namespace std::string_view_literals;

		//named based on which tile corners have terrain in them
		constexpr auto transition_names = std::array{
			"none"sv, // causes a tile skip
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
			"top-left_bottom-left-right"sv,
			"all"sv
		};

		//check against named layouts
		if (s.size() == 1u)
		{
			if (s[0] == "default"sv ||
				s[0] == "war3"sv)
				return { { std::begin(war3_layout), std::end(war3_layout) }, layout_type::war3 };
		}

		std::vector<transition_tile_type> out;
		out.reserve(size(s));

		using underlying_t = std::underlying_type_t<transition_tile_type>;
		for (const auto &str : s)
		{
			assert(std::numeric_limits<underlying_t>::max() > std::size(transition_names));
			assert(std::size(transition_names) == std::size(transition_short_names));
			for (auto i = underlying_t{}; i < std::size(transition_names); ++i)
			{
				if (transition_names[i] == str ||
					transition_short_names[i] == str)
				{
					out.emplace_back(transition_tile_type{ i });
					break;
				}
			}

			//TODO: accept numbers?
			//		warn for missing str?
		}

		return { out, layout_type::custom };
	}

	static void parse_terrain_group(terrain &t, const data::parser_node &p, data::data_manager &d, const resources::terrain_settings& s)
	{
		// p:
		//	texture:
		//	left:
		//	top:
		//	tiles-per-row:
		//	tile_count:
		//	layout: //either war3, a single unique_id, or a set of tile_count unique_ids

		// TODO: strengthen the error checking here

		using namespace std::string_view_literals;
		using namespace data::parse_tools;

		const auto layout_str = get_sequence<string>(p, "layout"sv, {});
		auto [transitions, layout_type] = parse_layout(layout_str);

		const auto tile_count = get_scalar<int32>(p, "tile-count"sv, tile_count_auto_val);
		const auto tiles_per_row = get_scalar<int32>(p, "tiles-per-row"sv, {});

		if (tiles_per_row < 1)
		{
			// TODO: need to abort this terrain group
			LOGERROR("a terrain group must provide tiles-per-row");
			// TODO: throw something
		}

		const auto left = get_scalar<tile_size_t>(p, "left"sv, 0);
		const auto top = get_scalar<tile_size_t>(p, "top"sv, 0);

		const auto texture_id = get_unique(p, "texture"sv, unique_id::zero);
		// TODO: handle texture_id == zero
		assert(texture_id);
		const auto texture = std::invoke(hades::detail::make_texture_link, d, texture_id, t.id);

		// store the parsed information for this terrain group
		using layout_type_enum = terrain::terrain_source::layout_type;
		using stored_layout = terrain::terrain_source::stored_layout;
		auto layout = stored_layout{ layout_type };

		if (layout_type == layout_type_enum::custom)
			layout = transitions;

		t.terrain_source_groups.emplace_back(terrain::tile_source{ texture_id, left, top, tiles_per_row,
			tile_count }, std::move(layout));

		//make transitions list match the length of tile_count
		//looping if needed
		if (tile_count != -1)
		{
			const auto count = unsigned_cast(tile_count);
			// take a copy of transitions, so that the iterators aren't invalidated while copying
			const std::vector rng = transitions;
			//copy the range until it is longer than tile_count
			while (count > std::size(transitions))
				std::ranges::copy(rng, std::back_inserter(transitions));

			//trim the vector back to tile_count length
			assert(count <= std::size(transitions));
			transitions.resize(count);
		}

		add_tiles_to_terrain(t, { left, top }, texture, transitions, tiles_per_row, s);
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
		//				tiles-per-row: <// number of tiles per row; required
		//				tile-count: <// total amount of tiles in tileset; default: tiles_per_row
		//			}
		//		terrain:
		//			- {
		//				texture: <// as above
		//				left: <// as above
		//				top: <// as above
		//				tiles-per-row: <// as above
		//				tile-count: <// optional; default is the length of layout
		//				layout: //either war3, a single unique_id, or a set of tile_count unique_ids; required
		//						// see parse_layout()::transition_names for a list of unique_ids
		//			}

		const auto terrains_list = p.get_children();
		auto settings = d.get<resources::terrain_settings>(id::terrain_settings, data::no_load);
		assert(settings);

		const auto tile_size = settings->tile_size;

		for (const auto &terrain_node : terrains_list)
		{
			using namespace std::string_view_literals;
			const auto name = terrain_node->to_string();
			const auto id = d.get_uid(name);

			auto terrain = d.find_or_create<resources::terrain>(id, m, terrains_str);
			assert(terrain);
			//parse_tiles will fill in the tags and tiles
			resources::detail::parse_tiles(*terrain, tile_size, *terrain_node, d);

			const auto terrain_n = terrain_node->get_child("terrain"sv);
			if (terrain_n)
			{
				const auto terrain_group = terrain_n->get_children();
				for (const auto &group : terrain_group)
					parse_terrain_group(*terrain, *group, d, *settings);
			}

			settings->tilesets.emplace_back(d.make_resource_link<resources::tileset>(id, id::terrain_settings));
			settings->terrains.emplace_back(d.make_resource_link<resources::terrain>(id, id::terrain_settings));
		}

		remove_duplicates(settings->terrains);
		remove_duplicates(settings->tilesets);
	}

	static void parse_terrainset(unique_id mod, const data::parser_node &n, data::data_manager &d)
	{
		//terrainsets:
		//	name: 
		//		terrain: [terrains, terrains, terrains] // not a mergable sequence, this will overwrite
		//		cliff: cliff-terrain-id

		auto settings = d.find_or_create<terrain_settings>(resources::get_tile_settings_id(), mod, terrain_settings_str);

		const auto list = n.get_children();

		for (const auto &terrainset_n : list)
		{
			const auto name = terrainset_n->to_string();
			const auto id = d.get_uid(name);

			auto t = d.find_or_create<terrainset>(id, mod, terrainsets_str);

			t->terrains = data::parse_tools::get_sequence(*terrainset_n, "terrains"sv, t->terrains, [id, &d](std::string_view s) {
				const auto i = d.get_uid(s);
				return d.make_resource_link<terrain>(i, id);
			});

			t->cliff_type = data::parse_tools::get_scalar(*terrainset_n, "cliff"sv, t->cliff_type, [id, &d](std::string_view s) {
				const auto i = d.get_uid(s);
				return d.make_resource_link<cliff_style>(i, id);
			});

			settings->terrainsets.emplace_back(d.make_resource_link<terrainset>(id, resources::get_tile_settings_id()));
		}

		remove_duplicates(settings->terrainsets);
	}

	static void parse_terrain_settings(unique_id mod, const data::parser_node& n, data::data_manager& d)
	{
		// TODO: this is quite out of date
		//		mostly the terrains and terrainsets
		//tile-settings:
		//  tile-size: 32
		//	error-tileset: uid
		//	empty-tileset: uid
		//  editor-terrain: uid
		//  empty-terrain: uid
		//	empty-terrainset: uid
		//  height-default: uint8
		//	height-min: uint8
		//	height-max: uint8
		//	cliff-default: uint8
		//	cliff-min: uint8
		//	cliff-max: uint8
		//	cliff-height: uint8

        auto s = d.find_or_create<resources::terrain_settings>(id::terrain_settings, mod, terrain_settings_str);
		assert(s);

		s->tile_size = data::parse_tools::get_scalar(n, "tile-size"sv, s->tile_size);

		const auto error_tset = data::parse_tools::get_unique(n, "error-tileset"sv, unique_zero);
		if (error_tset)
			s->error_tileset = d.make_resource_link<resources::tileset>(error_tset, id::terrain_settings);

		const auto empty_tset = data::parse_tools::get_unique(n, "empty-tileset"sv, unique_zero);
		if (empty_tset)
			s->empty_tileset = d.make_resource_link<resources::tileset>(empty_tset, id::terrain_settings);

		const auto editor_terrain = data::parse_tools::get_unique(n, "editor-terrain"sv, unique_zero);
		if (editor_terrain)
			s->editor_terrain = d.make_resource_link<resources::terrain>(editor_terrain, id::terrain_settings);

		const auto debug_cliffs = data::parse_tools::get_unique(n, "cliff-debug-terrain"sv, unique_zero);
		if (debug_cliffs)
			s->cliff_overlay_terrain = d.make_resource_link<resources::terrain>(debug_cliffs, id::terrain_settings);
		
		const auto debug_ramps = data::parse_tools::get_unique(n, "ramp-debug-terrain"sv, unique_zero);
		if (debug_ramps)
			s->ramp_overlay_terrain = d.make_resource_link<resources::terrain>(debug_ramps, id::terrain_settings);

		const auto grid_terrain = data::parse_tools::get_unique(n, "grid-terrain"sv, unique_zero);
		if (grid_terrain)
			s->grid_terrain = d.make_resource_link<resources::terrain>(grid_terrain, id::terrain_settings);

		const auto empty_terrain = data::parse_tools::get_unique(n, "empty-terrain"sv, unique_zero);
		if (empty_terrain)
			s->empty_terrain = d.make_resource_link<resources::terrain>(empty_terrain, id::terrain_settings);

		const auto empty_terrainset = data::parse_tools::get_unique(n, "empty-terrainset"sv, unique_zero);
		if (empty_terrainset)
			s->empty_terrainset = d.make_resource_link<resources::terrainset>(empty_terrainset, id::terrain_settings);

		s->height_default = data::parse_tools::get_scalar(n, "height-default"sv, s->height_default);
		s->height_min = data::parse_tools::get_scalar(n, "height-min"sv, s->height_min);
		s->height_max = data::parse_tools::get_scalar(n, "height-max"sv, s->height_max);
		s->cliff_default = data::parse_tools::get_scalar(n, "cliff-default"sv, s->cliff_default);
		s->cliff_min = data::parse_tools::get_scalar(n, "cliff-min"sv, s->cliff_min);
		s->cliff_max = data::parse_tools::get_scalar(n, "cliff-max"sv, s->cliff_max);
		s->cliff_height = data::parse_tools::get_scalar(n, "cliff-height"sv, s->cliff_height);

		return;
	}

	static void parse_cliff_styles(unique_id mod, const data::parser_node& n, data::data_manager& d)
	{
		//cliff-types:
		//	name:
		//		face-terrain: uid
		//		surface-terrain: uid

		for (const auto& type : n.get_children())
		{
			const auto name = type->to_string();
			const auto id = d.get_uid(name);
			auto new_style = d.find_or_create<cliff_style>(id, mod, cliff_styles_str);

			const auto to_terrain_link = [id, &d](auto str) {
				const auto i = d.get_uid(str);
				return d.make_resource_link<terrain>(i, id);
			};

			new_style->cliff_face_terrain = data::parse_tools::get_scalar(*type, cliff_face_terrain_str, new_style->cliff_face_terrain, to_terrain_link);
			new_style->cliff_surface_terrain = data::parse_tools::get_scalar(*type, cliff_surface_terrain_str, new_style->cliff_surface_terrain, to_terrain_link);
		}

		return;
	}

	const terrain_settings *get_terrain_settings()
	{
		return data::get<terrain_settings>(id::terrain_settings);
	}

	unique_id get_terrain_settings_id() noexcept
	{
		return id::terrain_settings;
	}

	// TODO: constexpr
	std::string_view get_empty_terrainset_name() noexcept
	{
		return empty_terrainset_str;
	}

	const terrain *get_empty_terrain(const terrain_settings &s)
	{
		return s.empty_terrain.get();
	}

	const terrain* get_background_terrain()
	{
		const auto settings = get_terrain_settings();
		assert(settings->empty_terrain);
		return settings->background_terrain.get();
	}

	unique_id get_background_terrain_id() noexcept
	{
		return background_terrain_id;
	}

	const terrainset* get_empty_terrainset()
	{
		const auto settings = get_terrain_settings();
		assert(settings && settings->empty_terrainset);
		return settings->empty_terrainset.get();
	}

	std::vector<tile>& get_transitions(terrain &t, transition_tile_type ty, const resources::terrain_settings& s)
	{
		return get_transitions_impl(ty, t, s);
	}

	const std::vector<tile>& get_transitions(const terrain &t, transition_tile_type ty, const resources::terrain_settings& s)
	{
		return get_transitions_impl(ty, t, s);
	}

	tile get_random_tile(const terrain &t, transition_tile_type type, const resources::terrain_settings& s)
	{
		const auto& vec = get_transitions(t, type, s);
		if (empty(vec))
			return s.error_tileset->tiles.front();
		return *random_element(std::begin(vec), std::end(vec));
	}

	std::optional<tile> try_get_random_tile(const terrain& t, transition_tile_type type, const resources::terrain_settings& s)
	{
		const auto& vec = get_transitions(t, type, s);
		if (empty(vec))
			return {};
		return *random_element(std::begin(vec), std::end(vec));
	}
}
