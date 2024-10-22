#include "hades/terrain.hpp"

#include <map>
#include <span>
#include <string>

#include "hades/data.hpp"
#include "hades/deflate.hpp"
#include "hades/parser.hpp"
#include "hades/random.hpp"
#include "hades/table.hpp"
#include "hades/tiles.hpp"
#include "hades/writer.hpp"

using namespace std::string_literals;
using namespace std::string_view_literals;

constexpr auto empty_cliff = hades::terrain_map::cliff_info{ false, false, false, false };

auto background_terrain_id = hades::unique_zero;
auto editor_terrainset_id = hades::unique_zero;

namespace hades::resources
{
	static void parse_terrain(unique_id, const data::parser_node&, data::data_manager&);
	static void parse_terrainset(unique_id, const data::parser_node&, data::data_manager&);
	static void parse_terrain_settings(unique_id, const data::parser_node&, data::data_manager&);
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

	void register_terrain_resources(data::data_manager &d, detail::make_texture_link_f func)
	{
		detail::make_texture_link = func;

		//create the terrain settings
		//and empty terrain first,
		//so that tiles can use them as tilesets without knowing
		//that they're really terrains
		id::terrain_settings = d.get_uid(resources::get_tile_settings_name());
		auto terrain_settings_res = d.find_or_create<resources::terrain_settings>(id::terrain_settings, {}, terrain_settings_str);
		const auto empty_tileset_id = d.get_uid(resources::get_empty_tileset_name());
		auto empty = d.find_or_create<resources::terrain>(empty_tileset_id, {}, terrains_str);
		
		//fill all the terrain tile lists with a default constructed tile
		apply_to_terrain(*empty, [empty](std::vector<resources::tile>&v) {
			v.emplace_back(resources::tile{ {}, 0u, 0u, empty });
		});
		
		terrain_settings_res->empty_terrain = d.make_resource_link<resources::terrain>(empty_tileset_id, id::terrain_settings);
		terrain_settings_res->empty_tileset = d.make_resource_link<resources::tileset>(empty_tileset_id, id::terrain_settings);

		const auto empty_tset_id = d.get_uid(resources::get_empty_terrainset_name());
		auto empty_tset = d.find_or_create<resources::terrainset>(empty_tset_id, {}, terrainsets_str);
		empty_tset->terrains.emplace_back(terrain_settings_res->empty_terrain);

		terrain_settings_res->empty_terrainset = d.make_resource_link<resources::terrainset>(empty_tset_id, id::terrain_settings);

		editor_terrainset_id = make_unique_id();
		const auto editor_terrainset = d.find_or_create<resources::terrainset>(editor_terrainset_id, {}, terrainsets_str);
		terrain_settings_res->editor_terrainset = d.make_resource_link<resources::terrainset>(editor_terrainset->id, id::terrain_settings);

		// the exact same as empty tile
		// since its now used as 'air' tiles for higher layers
		// it can be made unpathable by games, so that unwalkable
		// background can be visible through the map
		background_terrain_id = d.get_uid("terrain-background"sv);
		auto background_terrain = d.find_or_create<resources::terrain>(background_terrain_id, {}, terrains_str);
		
		// add empty tiles to all the tile arrays
		apply_to_terrain(*background_terrain, [background_terrain](std::vector<resources::tile>& v) {
			v.emplace_back(resources::tile{ {}, 0u, 0u, background_terrain });
			return;
		});

		terrain_settings_res->background_terrain = d.make_resource_link<resources::terrain>(background_terrain_id, id::terrain_settings);

		//register tile resources
		register_tiles_resources(d, func);

		//replace the tileset and tile settings parsers
		//register tile resources
		d.register_resource_type(resources::get_tile_settings_name(), resources::parse_terrain_settings);
		d.register_resource_type(terrain_settings_str, resources::parse_terrain_settings);
		d.register_resource_type(resources::get_tilesets_name(), resources::parse_terrain);
		d.register_resource_type(terrainsets_str, resources::parse_terrainset);
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

	template<typename InputIt>
	static tile_map generate_layer(const std::vector<const resources::terrain*> &v,
		tile_index_t w, InputIt first, InputIt last, const resources::terrain_settings& s)
	{
		//NOTE: w2 and h are 0 based, and one less than vertex width and height
		//this is good, it means they are equal to the tile width/height
		assert(!std::empty(v));
		const auto vertex_h = integer_cast<tile_index_t>(size(v)) / (w + 1);
		const auto h = vertex_h - 1;

		auto out = tile_map{};
		out.width = w;
		out.tilesets = std::vector<const resources::tileset*>{
			resources::get_empty_terrain(s),
			first->get()
		};

		const auto size = w * h;

		for (auto i = tile_index_t{}; i < size; ++i)
		{
			const auto [x, y] = to_2d_index(i, w);
			assert(x < w);

			const auto corners = get_terrain_at_tile(v, w + 1, { integer_cast<int32>(x), integer_cast<int32>(y) }, s);
			const auto type = get_transition_type(corners, first, last);
			const auto tile = resources::get_random_tile(**first, type, s);
			out.tiles.emplace_back(get_tile_id(out, tile));
		}

		return out;
	}

	static std::vector<tile_map> generate_terrain_layers(const resources::terrainset *t,
		const std::vector<const resources::terrain*> &v, tile_index_t width, const resources::terrain_settings& s)
	{
		auto out = std::vector<tile_map>{};

		const auto end = std::crend(t->terrains);
		for (auto iter = std::crbegin(t->terrains); iter != end; ++iter)
			out.emplace_back(generate_layer(v, width, iter, end, s));

		std::reverse(std::begin(out), std::end(out));

		return out;
	}

	bool is_valid(const raw_terrain_map &r)
	{
		if (std::empty(r.tile_layer.tiles) ||
			r.tile_layer.width < 1)
		{
			LOGWARNING("raw terrain map must have a valid tile layer in order to compute size");
			return false;
		}

		//check the the map has a valid terrain vertex
		const auto size = get_size(r.tile_layer);
		const auto terrain_size = terrain_vertex_position{
            size.x + 1,
            size.y + 1
		};

		const auto vertex_length = integer_cast<std::size_t>(terrain_size.x) * integer_cast<std::size_t>(terrain_size.y);

		if (!std::empty(r.terrain_vertex) &&
			integer_cast<decltype(vertex_length)>(std::size(r.terrain_vertex)) != vertex_length)
		{
			LOGWARNING("raw map terrain vertex list should be the same number of tiles as the tile_layer, or it should be empty.");
			return false;
		}

		const auto tile_count = integer_cast<std::size_t>(size.x) * integer_cast<std::size_t>(size.y);
		if (std::size(r.heightmap) != tile_count * 6)
		{
			LOGWARNING("Heightmap must contain 6 samples for each tile.");
			return false;
		}

		if (std::size(r.cliff_data) != tile_count)
		{
			LOGWARNING("tile cliff information must exist for each tile");
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

		if (!std::empty(r.cliffs.tiles))
		{
			if(std::size(r.cliffs.tiles) != std::size(r.tile_layer.tiles) * 4)
			{
				LOGWARNING("raw terrain map, if tile data for cliffs is present, then this layer must be 4 times the size as the tile-layer");
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

		const auto tilemap_size = get_size(r.tile_layer);

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
	constexpr auto terrain_layers_str = "terrain-layers"sv;
	constexpr auto level_tiles_layer_str = "tile-layer"sv;
	constexpr auto level_cliff_layer_str = "cliff-layer"sv;

	void write_raw_terrain_map(const raw_terrain_map & m, data::writer & w)
	{
		// we start in a terrain: node

		//terrainset
		//terrain vertex
		// tile layers

		w.write(terrainset_str, m.terrainset);

		const auto compressed = zip::deflate(m.terrain_vertex);
		w.write(terrain_vertex_str, base64_encode(compressed));
		const auto compressed_height = zip::deflate(m.heightmap);
		w.write(terrain_height_str, base64_encode(compressed_height));

		// TODO: triangles and cliffs

		w.start_sequence(terrain_layers_str); 
		for (const auto &l : m.terrain_layers)
		{
			w.start_map();
			write_raw_map(l, w);
			w.end_map();
		}
		w.end_sequence();

		w.start_map(level_tiles_layer_str);
		write_raw_map(m.tile_layer, w);
		w.end_map();

		w.start_map(level_cliff_layer_str);
		write_raw_map(m.cliffs, w);
		w.end_map();

		assert(false && "need to write triangle and cliff data");
	}

	raw_terrain_map read_raw_terrain_map(const data::parser_node &p, std::size_t layer_size, std::size_t vert_size)
	{
		//terrain:
		//	terrainset:
		//  vertex_encoding:
		//	terrain_vertex:
		//	vertex_height:
		//	terrain_layers:
		//	cliff_info:
		//	triangle_shape:

		raw_terrain_map out; // uninitialized

		out.terrainset = data::parse_tools::get_unique(p, terrainset_str, unique_zero);
		auto terrain_vertex = std::vector<terrain_id_t>{};

		if (auto vertex_node = p.get_child(terrain_vertex_str); 
			vertex_node)
		{
			if(vertex_node->is_sequence())
				terrain_vertex = vertex_node->to_sequence<terrain_id_t>();
			else
			{
				const auto map_encoded = vertex_node->to_string();
				const auto bytes = base64_decode<std::byte>(map_encoded);
				terrain_vertex = zip::inflate<terrain_id_t>(bytes, vert_size * sizeof(terrain_id_t));
			}
		}

		out.terrain_vertex = std::move(terrain_vertex);

		auto terrain_height = std::vector<std::uint8_t>{};

		if (auto height_node = p.get_child(terrain_height_str);
			height_node)
		{
			if (height_node->is_sequence())
				terrain_height = height_node->to_sequence<std::uint8_t>();
			else
			{
				const auto map_encoded = height_node->to_string();
				const auto bytes = base64_decode<std::byte>(map_encoded);
				terrain_height = zip::inflate<std::uint8_t>(bytes, vert_size * sizeof(std::uint8_t));
			}
		}
		
		out.heightmap = std::move(terrain_height);

		// TODO: cliff_data
		//  triangles
		//			cliffs
		//

		auto layers = std::vector<raw_map>{};

		const auto layer_node = p.get_child(terrain_layers_str);
		for (const auto& l : layer_node->get_children())
			layers.emplace_back(read_raw_map(*l, layer_size));

		out.terrain_layers = std::move(layers);
		if(const auto tile_node = p.get_child(level_tiles_layer_str); tile_node)
			out.tile_layer = read_raw_map(*tile_node, layer_size);

		if (const auto cliff_node = p.get_child(level_cliff_layer_str); cliff_node)
			out.cliffs = read_raw_map(*cliff_node, layer_size * 4);

		return out;
	}

	// generate tiles for the cliffs
	static tile_map generate_cliff_layer(const terrain_map& map,
		const resources::terrain_settings& s)
	{
		const auto world_size_tiles = get_size(map) * 4;
		const auto& empty = resources::get_empty_tile(s);

		if (!map.terrainset->cliff_terrain)
			return make_map(world_size_tiles, empty, s);

		const auto& cliff_terrain = *map.terrainset->cliff_terrain.get();
		
		auto layer = make_map(world_size_tiles, empty, s);
		const auto size = integer_cast<tile_index_t>(std::size(map.cliff_data));
		for (auto i = tile_index_t{}; i < size; ++i)
		{
			const auto tile_pos = from_tile_index(i, map);

			const auto our_cliffs = get_cliff_info(i, map);
			const auto cliff_corners = get_cliffs_corners(tile_pos, map);
			
			const auto index = i * 4;
			using tile_type = terrain_map::cliff_layer_layout;
			// top of tile
			const auto top_trans = get_transition_type(cliff_corners);
			if (top_trans != resources::transition_tile_type::none)
			{
				const auto& top_tile = resources::get_random_tile(cliff_terrain, top_trans, s);
				place_tile(layer, index + enum_type(tile_type::surface), top_tile, s);
			}

			// diag
			if (our_cliffs.diag)
			{
				const auto& tile = resources::get_random_tile(cliff_terrain, resources::transition_tile_type::all, s);
				place_tile(layer, index + enum_type(tile_type::diag), tile, s);
			}
			// right
			if (our_cliffs.right)
			{
				const auto& tile = resources::get_random_tile(cliff_terrain, resources::transition_tile_type::all, s);
				place_tile(layer, index + enum_type(tile_type::right), tile, s);
			}
			// bottom
			if (our_cliffs.bottom)
			{
				const auto& tile = resources::get_random_tile(cliff_terrain, resources::transition_tile_type::all, s);
				place_tile(layer, index + enum_type(tile_type::bottom), tile, s);
			}
		}

		return layer;
	}

	// TODO: require terrain_settings as a parameter
	template<typename Raw>
		requires std::same_as<raw_terrain_map, std::decay_t<Raw>>
	terrain_map to_terrain_map_impl(Raw &&r)
	{
		if (!is_valid(r))
			throw terrain_error{ "raw terrain map is not valid" };

		auto m = terrain_map{};

		m.terrainset = data::get<resources::terrainset>(r.terrainset);
		m.heightmap = std::forward<Raw>(r).heightmap;
		m.cliff_data = std::forward<Raw>(r).cliff_data;

		//tile layer is required to be valid
		m.tile_layer = to_tile_map(r.tile_layer);
		const auto size = get_size(m.tile_layer);

		const auto settings = resources::get_terrain_settings();
		assert(settings);

		// cliff data is also required to be valid
		if(empty(r.cliffs.tiles))
			m.cliffs = generate_cliff_layer(m, *settings);
		else
			m.cliffs = to_tile_map(r.cliffs);

		
		const auto empty = settings->empty_terrain.get();
		//if the terrain_vertex isn't present, then fill with empty
		if (std::empty(r.terrain_vertex))
			m.terrain_vertex = std::vector<const resources::terrain*>(integer_cast<std::size_t>((size.x + 1) * (size.y + 1)), empty);
		else
		{
			std::transform(std::begin(r.terrain_vertex), std::end(r.terrain_vertex),
				std::back_inserter(m.terrain_vertex), [empty, &terrains = m.terrainset->terrains](terrain_id_t t) {
					if (t == terrain_id_t{})
						return empty;

					assert(t <= std::size(terrains));
					return terrains[t - 1u].get();
				});
		}

		// if the terrain layers are empty then generate them
		m.terrain_layers = generate_terrain_layers(m.terrainset, m.terrain_vertex, size.x, *settings);

		if (std::empty(r.terrain_layers))
		{
			if (m.terrainset == settings->empty_terrainset.get())
				m.terrain_layers.emplace_back(m.tile_layer);
		}
		else
		{
			for (auto& l : m.terrain_layers)
			{
				const auto terrain = std::find_if(begin(l.tilesets), end(l.tilesets), [empty](auto& t) {
					return t != empty;
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

		// TODO: this is a very expensive assert
		assert(is_valid(to_raw_terrain_map(m)));

		return m;
	}

	terrain_map to_terrain_map(const raw_terrain_map &r)
	{
		return to_terrain_map_impl(r);
	}

	terrain_map to_terrain_map(raw_terrain_map&& r)
	{
		return to_terrain_map_impl(std::move(r));
	}

	// TODO: require terrain settings as a parameter
	template<typename Map>
		requires std::same_as<terrain_map, std::decay_t<Map>>
	static raw_terrain_map to_raw_terrain_map_impl(Map &&t)
	{
		auto m = raw_terrain_map{};

		m.terrainset = t.terrainset->id;
		m.heightmap = std::forward<Map>(t).heightmap;
		m.cliff_data = std::forward<Map>(t).cliff_data;

		//build a replacement lookup table
		auto t_map = std::map<const resources::terrain*, terrain_id_t>{};

		const auto s = resources::get_terrain_settings();
		assert(s);
		const auto empty = resources::get_empty_terrain(*s);
		//add the empty terrain with the index 0
		t_map.emplace(empty, terrain_id_t{});

		//add the rest of the terrains, offset the index by 1
		for (auto i = terrain_id_t{}; i < std::size(t.terrainset->terrains); ++i)
			t_map.emplace(t.terrainset->terrains[i].get(), i + 1u);

		m.terrain_vertex.reserve(size(t.terrain_vertex));
		std::transform(std::begin(t.terrain_vertex), std::end(t.terrain_vertex),
			std::back_inserter(m.terrain_vertex), [t_map](const resources::terrain* t) {
				return t_map.at(t);
			});

		m.tile_layer = to_raw_map(std::forward<Map>(t).tile_layer);
		m.cliffs = to_raw_map(std::forward<Map>(t).cliffs);

		for (auto&& tl : std::forward<Map>(t).terrain_layers)
			m.terrain_layers.emplace_back(to_raw_map(std::forward<std::remove_reference_t<decltype(tl)>>(tl)));

		if constexpr (std::is_rvalue_reference_v<decltype(t)>)
		{
			t.terrain_layers.clear();
			t.terrain_vertex.clear();
		}

		return m;
	}

	raw_terrain_map to_raw_terrain_map(const terrain_map &t)
	{
		return to_raw_terrain_map_impl(t);
	}

	raw_terrain_map to_raw_terrain_map(terrain_map&& t)
	{
		return to_raw_terrain_map_impl(std::move(t));
	}

	terrain_map make_map(const tile_position size, const resources::terrainset *terrainset,
		const resources::terrain *t, const resources::terrain_settings& s)
	{
		assert(terrainset);
		assert(t);

		auto map = terrain_map{};
		map.terrainset = terrainset;
        map.terrain_vertex.resize(integer_cast<std::size_t>((size.x + 1) * (size.y + 1)), t);
		const auto tile_count = integer_cast<std::size_t>(size.x) * integer_cast<std::size_t>(size.y);
		map.heightmap.resize(tile_count * 6, s.height_default);
		map.cliff_data.resize(tile_count, empty_cliff);

		const auto& empty_tile = resources::get_empty_tile(s);
		const auto empty_layer = make_map(size, empty_tile, s);

		// empty cliff layer should just be empty tiles
		map.cliffs = make_map(size * 2, empty_tile, s);

		if (t != resources::get_empty_terrain(s))
		{
			//fill in the correct terrain layer
			const auto index = get_terrain_id(terrainset, t);

			const auto end = std::size(terrainset->terrains);
			map.terrain_layers.reserve(std::size(terrainset->terrains));
			for (auto i = std::size_t{}; i < end; ++i)
			{
				if (i == index)
				{
					map.terrain_layers.emplace_back(make_map(
						size,
						resources::get_random_tile(*terrainset->terrains[i],
							resources::transition_tile_type::all, s),
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

		map.tile_layer = empty_layer;
		assert(is_valid(to_raw_terrain_map(map)));

		return map;
	}

	terrain_index_t get_width(const terrain_map &m)
	{
		return m.tile_layer.width + 1;
	}

	tile_position get_size(const terrain_map& t)
	{
		return get_size(t.tile_layer);
	}

	tile_position get_vertex_size(const terrain_map& t)
	{
		return get_size(t.tile_layer) + tile_position{ 1, 1 };
	}

	void copy_heightmap(terrain_map& target, const terrain_map& src)
	{
		if (size(target.heightmap) != size(src.heightmap))
			throw terrain_error{ "Tried to copy heightmaps between incompatible maps" };
		target.heightmap = src.heightmap;
		return;
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

	const resources::terrain *get_corner(const tile_corners &t, rect_corners c) noexcept
	{
		return t[enum_type(c)];
	}

	resources::transition_tile_type get_transition_type(const std::array<bool, 4u> &arr) noexcept
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

	tile_corners get_terrain_at_tile(const terrain_map &m, tile_position p, const resources::terrain_settings& s)
	{
		const auto w = get_width(m);
		return get_terrain_at_tile(m.terrain_vertex, w, p, s);
	}

	triangle_height_data get_height_for_triangles(const tile_position p, const terrain_map& m)
	{
		const auto index = to_tile_index(p, m);
		const auto h = detail::get_triangle_height(index, m);
		const auto& c = get_cliff_info(index, m);
		triangle_height_data ret [[indeterminate]];
		std::ranges::copy(h, begin(ret.height));
		ret.triangle_type = c.triangle_type;
		return ret;
	}

	void set_height_for_triangles(const tile_position p, terrain_map& m, const triangle_height_data t)
	{
		const auto index = to_tile_index(p, m);
		auto h = detail::get_triangle_height(index, m);
		std::ranges::copy(t.height, begin(h));
		return;
	}

	std::array<std::uint8_t, 4> get_max_height_in_corners(const tile_position tile_index, const terrain_map& map)
	{
		const auto tris = get_height_for_triangles(tile_index, map);
		return get_max_height_in_corners(tris);
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

	std::array<std::uint8_t, 4> get_max_height_in_corners(const triangle_height_data& tris) noexcept
	{
		using max_func = const std::uint8_t& (*)(const std::uint8_t&, const std::uint8_t&) noexcept;
		constexpr max_func max = &std::max<std::uint8_t>;
		
		return {
			detail::read_triangles_as_quad(tris, rect_corners::top_left, max),
			detail::read_triangles_as_quad(tris, rect_corners::top_right, max),
			detail::read_triangles_as_quad(tris, rect_corners::bottom_right, max),
			detail::read_triangles_as_quad(tris, rect_corners::bottom_left, max),
		};
	}

	terrain_map::cliff_info get_cliff_info(const tile_index_t ind, const terrain_map& m) noexcept
	{
		assert(ind < size(m.cliff_data));
		return m.cliff_data[ind];
	}

	static void set_cliff_info(const tile_index_t ind, terrain_map& m, const terrain_map::cliff_info c)
	{
		assert(ind < size(m.cliff_data));
		m.cliff_data[ind] = c;
		// TODO: only do this for nearby tiles
		// TODO: look into getting settings as a parameter
		m.cliffs = generate_cliff_layer(m, *resources::get_terrain_settings());
		return;
	}

	terrain_map::cliff_info get_cliff_info(const tile_position p, const terrain_map& m)
	{
		const auto ind = to_tile_index(p, m);
		return get_cliff_info(ind, m);
	}

	void set_cliff_info_tmp(tile_position p, terrain_map& m, terrain_map::cliff_info c) // TODO: temp remove
	{
		const auto ind = to_tile_index(p, m);
		return set_cliff_info(ind, m, c);
	}

	std::array<bool, 6> get_adjacent_cliffs(const tile_position p, const terrain_map& m)
	{
		const auto find_cliffs = [](const tile_position p, const terrain_map& m) {
			if (p.x < 0 || p.y < 0)
				return empty_cliff;

			return get_cliff_info(p, m);
			};

		const auto our_cliffs = get_cliff_info(p, m);

		const auto above = find_cliffs(p - tile_position{ 0, 1 }, m);
		const auto left = find_cliffs(p - tile_position{ 1, 0 }, m);

		return {
			above.bottom, // top
			our_cliffs.right,
			our_cliffs.bottom,
			left.right, // left
			our_cliffs.diag_uphill,
			our_cliffs.diag_downhill,
		};
	}

	// NOTE: this can make some processing easier,
	//		but removes some passive cliff direction information
	static constexpr tile_edge normalise(const tile_edge edge) noexcept
	{
		switch (edge.edge)
		{
		case rect_edges::top:
			return { edge.position - tile_position{ 0, 1 }, rect_edges::bottom };
		case rect_edges::left:
			return { edge.position - tile_position{ 1, 0 }, rect_edges::right };
		default:
			return edge;
		}
	}

	std::array<bool, 4> get_cliffs_corners(const tile_position p, const terrain_map& m)
	{
		// TODO: this needs to include checks against adjacent tiles, 
		//			perhaps an extra function needs to do 

		// see for_each_tile_corner

		const auto find_cliffs = [](const tile_position p, const terrain_map& m) {
			if (p.x < 0 || p.y < 0)
				return empty_cliff;

			return get_cliff_info(p, m);
			};

		const auto our_cliffs = get_cliff_info(p, m);
		const auto above = find_cliffs(p - tile_position{ 0, 1 }, m);
		const auto left = find_cliffs(p - tile_position{ 1, 0 }, m);

		// TODO: maybe able to make this simpler using .diag_uphill and .diag_downhill
		if (our_cliffs.triangle_type == terrain_map::triangle_uphill)
		{
			return {
				above.bottom || left.right,								// top left
				above.bottom || our_cliffs.right || our_cliffs.diag,	// top right
				our_cliffs.bottom || our_cliffs.right,					// bottom right
				our_cliffs.bottom || left.right || our_cliffs.diag,		// bottom left
			};
		}
		else
		{
			return {
				above.bottom || left.right || our_cliffs.diag,				// top left
				above.bottom || our_cliffs.right,							// top right
				our_cliffs.bottom || our_cliffs.right || our_cliffs.diag,	// bottom right
				our_cliffs.bottom || left.right								// bottom left
			};
		}
	}

	std::optional<tile_edge> get_next_cliff(const terrain_vertex_position v, const tile_edge edge, const terrain_map& map) noexcept
	{
		auto out = std::optional<tile_edge>{};
		const auto check_downhill = [&](const auto pos, const auto cliffs) noexcept {
			if (cliffs.diag_downhill || (cliffs.triangle_type == terrain_map::triangle_downhill && cliffs.diag))
			{
				auto e = tile_edge{ pos, rect_edges::downhill };
				if (e != edge)
					out = e;
			}
		};

		const auto check_uphill = [&](const auto pos, const auto cliffs) noexcept {
			if (cliffs.diag_uphill || (cliffs.triangle_type == terrain_map::triangle_uphill && cliffs.diag))
			{
				auto e = tile_edge{ pos, rect_edges::uphill };
				if (e != edge)
					out = e;
			}
		};

		const auto check_bottom = [&](const auto pos, const auto cliffs) noexcept {
			if (cliffs.bottom)
			{
				auto e = tile_edge{ pos, rect_edges::bottom };
				if (e != edge)
					out = e;
			}
		};

		const auto check_right = [&](const auto pos, const auto cliffs) noexcept {
			if (cliffs.right)
			{
				auto e = tile_edge{ pos, rect_edges::right };
				if (e != edge)
					out = e;
			}
		};

		for_each_safe_adjacent_corner(map, v, [&](const tile_position pos, const rect_corners corner) noexcept {
			const auto cliffs = get_cliff_info(pos, map);
			switch (corner)
			{
			case rect_corners::top_right:
			{
				check_uphill(pos, cliffs);
				check_right(pos, cliffs);
			} break;
			case rect_corners::bottom_left:
			{
				check_uphill(pos, cliffs);
				check_bottom(pos, cliffs);
			} break;
			case rect_corners::bottom_right:
			{
				check_right(pos, cliffs);
				check_bottom(pos, cliffs);
			} [[fallthrough]];
			case rect_corners::top_left:
			{
				check_downhill(pos, cliffs);
			} break;
			default:
				assert(false);
			}
			return;
		});

		return out;
	}

	std::optional<tile_edge> get_next_edge(const terrain_vertex_position v, tile_edge e, const terrain_map& m) noexcept
	{
		const auto left = v.x == e.position.x ? -1 : 1;
		const auto top = v.y == e.position.y ? -1 : 1;

		switch (e.edge)
		{
			// travelling left / right
		case rect_edges::top:
			[[fallthrough]];
		case rect_edges::bottom:
			e.position += tile_position{ left, 0 };
			break;
			// travelling up / down
		case rect_edges::left:
			[[fallthrough]];
		case rect_edges::right:
			e.position += tile_position{ 0, top };
			break;
			// diags
		case rect_edges::uphill:
			[[fallthrough]];
		case rect_edges::downhill:
			e.position += tile_position{ left, top };
			break;
		default:
			e.position = bad_tile_position;
		}

		if (within_world(e.position, get_size(m)))
			return e;
		else
			return {};
	}

	bool is_tile_split(const tile_position p, const terrain_map& m)
	{
		const auto& cliff = get_cliff_info(p, m);
		return cliff.diag;
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

		const auto& tile_tags = get_tags_at(m.tile_layer, p);
		out.insert(end(out), begin(tile_tags), end(tile_tags));

		return out;
	}

	void resize_map_relative(terrain_map& m, const vector2_int top_left, const vector2_int bottom_right,
		const resources::terrain* t, const std::uint8_t height, const resources::terrain_settings& s)
	{
        const auto current_height = integer_cast<tile_index_t>(m.tile_layer.tiles.size()) / m.tile_layer.width;
		const auto current_width = m.tile_layer.width;

        const auto new_height = current_height - top_left.y + bottom_right.y;
        const auto new_width = current_width - top_left.x + bottom_right.x;

		const auto size = vector2_int{
            new_width,
            new_height
		};

		resize_map(m, size, { -top_left.x, -top_left.y }, t, height, s);
	}

	[[deprecated]]
	void resize_map_relative(terrain_map& m, vector2_int top_left, vector2_int bottom_right)
	{
		const auto settings = resources::get_terrain_settings();
		assert(settings);
		const auto terrain = settings->empty_terrain.get();
		resize_map_relative(m, top_left, bottom_right, terrain, settings->height_default, *settings);
	}

	void resize_map(terrain_map& m, const vector2_int s, const vector2_int o,
		const resources::terrain* t, const std::uint8_t height, const resources::terrain_settings& settings)
	{
		const auto old_size = get_vertex_size(m);
		//resize tile layer
		resize_map(m.tile_layer, s, o, settings);

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

	[[deprecated]]
	void resize_map(terrain_map& m, vector2_int size, vector2_int offset)
	{
		const auto settings = resources::get_terrain_settings();
		assert(settings);
		const auto terrain = settings->empty_terrain.get();
		resize_map(m, size, offset, terrain, settings->height_default, *settings);
	}

	[[deprecated]] static std::vector<tile_position> get_adjacent_tiles(terrain_vertex_position p)
	{
		return {
			//top row
			{p.x - 1, p.y -1},
			{p.x, p.y -1},
			//middle row
			{p.x - 1, p.y},
			p
		};
	}

	[[deprecated]] static std::vector<tile_position> get_adjacent_tiles(const std::vector<terrain_vertex_position> &v)
	{
		auto out = std::vector<tile_position>{};

		for (const auto& p : v)
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
	void place_tile(terrain_map &m, const tile_position p, const resources::tile &t,
		const resources::terrain_settings& s)
	{
		place_tile(m.tile_layer, p, t, s);
	}

	void place_tile(terrain_map &m, const std::vector<tile_position> &p,
		const resources::tile &t, const resources::terrain_settings& s)
	{
		place_tile(m.tile_layer, p, t, s);
	}

	static void place_terrain_internal(terrain_map &m, const terrain_vertex_position p,
		const resources::terrain *t)
	{
		assert(t);
		const auto index = to_1d_index(p, get_width(m));
        m.terrain_vertex[integer_cast<std::size_t>(index)] = t;
	}

	static void update_tile_layers_internal(terrain_map &m, const std::vector<tile_position> &positions,
		const resources::terrain_settings& s)
	{
		//remove tiles from the tile layer,
		//so that they dont obscure the new terrain
		place_tile(m.tile_layer, positions, resources::get_empty_tile(s), s);

		const auto begin = std::cbegin(m.terrainset->terrains);
		const auto end = std::cend(m.terrainset->terrains);
		auto terrain_iter = std::begin(m.terrainset->terrains);
		auto layer_iter = std::begin(m.terrain_layers);
		assert(std::size(m.terrain_layers) == std::size(m.terrainset->terrains));
        for (/*terrain_iter, layer_iter*/; terrain_iter != end; ++terrain_iter, ++layer_iter)
		{
			for (const auto& p : positions)
			{
				//get the terrain corners for this tile
				const auto corners = get_terrain_at_tile(m, p, s);

				const auto equal_terrain = [t = *terrain_iter](auto &&other_t){
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

	template<typename Position>
	static void update_tile_layers(terrain_map &m, Position p, const resources::terrain_settings& s)
	{
		// TODO: rewrite to not need get_adjacent_tiles as it allocates even in the worst case
		const auto t_p = get_adjacent_tiles(p);
		update_tile_layers_internal(m, t_p, s);
	}

	void place_terrain(terrain_map &m, terrain_vertex_position p, const resources::terrain *t,
		const resources::terrain_settings& s)
	{
		// TODO: check t is non null
		if (within_map(m, p))
		{
			place_terrain_internal(m, p, t);
			update_tile_layers(m, p, s);
		}
	}

	//positions outside the map will be ignored
	void place_terrain(terrain_map &m, const std::vector<terrain_vertex_position> &positions,
		const resources::terrain *t, const resources::terrain_settings& settings)
	{
		// TODO: check t is non null
		const auto s = get_vertex_size(m);
		for (const auto& p : positions)
			if (within_map(s, p))
				place_terrain_internal(m, p, t);

		update_tile_layers(m, positions, settings);
	}

	// TODO: mark static, internal only
	void swap_triangle_type(terrain_map& m, const tile_position p)
	{
		const auto index = to_tile_index(p, m);
		auto h = detail::get_triangle_height(index, m);
		auto c = get_cliff_info(p, m);
		// we should never do this if a tile contains a cliff
		assert(!c.diag && !c.diag_downhill && !c.diag_uphill); // TODO: throw terrain error (or logic_error)
		c.triangle_type = !c.triangle_type;
		if (c.triangle_type == terrain_map::triangle_uphill)
		{
			assert(h[0] == h[3] &&
				h[2] == h[4]); // TODO: throw terrain_error

			h[4] = h[1];
			h[3] = h[5];
			std::swap(h[5], h[2]);
		}
		else
		{
			assert(h[2] == h[3] &&
				h[1] == h[4]); // TODO: throw terrain_error

			h[3] = h[0];
			h[4] = h[5];
			std::swap(h[5], h[2]);
		}
		
		set_cliff_info(index, m, c);
		return;
	}

	bool is_tile_cliff(const terrain_map& m, tile_position p) noexcept
	{
		const auto c = get_cliff_info(p, m);
		return c.bottom || c.diag || c.right;
	}

	static constexpr tile_edge swap_cliff_facing(const tile_edge e) noexcept
	{
		using enum rect_edges;
		switch (e.edge)
		{
		case top:
			return { e.position - tile_position{ 0, 1 }, bottom };
		case left:
			return { e.position - tile_position{ 1, 0 }, right };
		case right:
			return { e.position + tile_position{ 1, 0 }, left };
		case bottom:
			return { e.position + tile_position{ 0, 1 }, top };
		case uphill:
			[[fallthrough]];
		case downhill:
			return { e.position, e.edge, !e.left_triangle };
		default:
			return { bad_tile_position, end };
		}
	}

	namespace detail
	{
		// if hint is first element, iterate over list in reverse
		// if hint is the last element then iterator over in order
		constexpr auto search_orders = std::array<std::array<tile_edge, 8>, 4>{
			std::array<tile_edge, 8>{
				// 0
				tile_edge{ {},						rect_edges::top},
				// 45
				tile_edge{ tile_position{ 0, -1 },	rect_edges::uphill },
				// 315
				tile_edge{ {},						rect_edges::downhill},
				// 90
				tile_edge{ tile_position{ 0, -1 },	rect_edges::left },
				// 270
				tile_edge{ {},						rect_edges::left},
				// 135
				tile_edge{ tile_position{ -1, -1 },	rect_edges::downhill },
				// 225
				tile_edge{ tile_position{ -1, 0 },	rect_edges::uphill },
				// 180
				tile_edge{ tile_position{ -1, 0 },	rect_edges::top }
			},	
			std::array<tile_edge, 8>{
				// 45
				tile_edge{ tile_position{ 0, -1 },	rect_edges::uphill },
				// 0
				tile_edge{ {},						rect_edges::top },
				// 90
				tile_edge{ tile_position{ 0, -1 },	rect_edges::left },
				// 315
				tile_edge{ {},						rect_edges::downhill },
				// 135
				tile_edge{ tile_position{ -1, -1 },	rect_edges::downhill },
				// 270
				tile_edge{ {},						rect_edges::left },
				// 180
				tile_edge{ tile_position{ -1, 0 },	rect_edges::top },
				// 225
				tile_edge{ tile_position{ -1, 0 },	rect_edges::uphill }
			},
			std::array<tile_edge, 8>{
				// 90
				tile_edge{ tile_position{ 0, -1 },	rect_edges::left },
				// 45
				tile_edge{ tile_position{ 0, -1 },	rect_edges::uphill },
				// 135
				tile_edge{ tile_position{ -1, -1 },	rect_edges::downhill },
				// 0
				tile_edge{ {},						rect_edges::top },
				// 180
				tile_edge{ tile_position{ -1, 0 },	rect_edges::top },
				// 315
				tile_edge{ {},						rect_edges::downhill },
				// 225
				tile_edge{ tile_position{ -1, 0 },	rect_edges::uphill },
				// 270
				tile_edge{ {},						rect_edges::left }
			},
			std::array<tile_edge, 8>{
				// 135
				tile_edge{ tile_position{ -1, -1 },	rect_edges::downhill },
				// 90
				tile_edge{ tile_position{ 0, -1 },	rect_edges::left },
				// 180
				tile_edge{ tile_position{ -1, 0 },	rect_edges::top },
				// 45
				tile_edge{ tile_position{ 0, -1 },	rect_edges::uphill },
				// 225
				tile_edge{ tile_position{ -1, 0 },	rect_edges::uphill },
				// 0
				tile_edge{ {},						rect_edges::top },
				// 270
				tile_edge{ {},						rect_edges::left },
				// 315
				tile_edge{ {},						rect_edges::downhill }
			}
		};
	}

	template<std::invocable<tile_edge> Func>
	static void for_each_adjacent_edge(const terrain_vertex_position p,
		std::optional<tile_edge> hint, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_edge>)
	{
		// make the seach order as though hint is the edge pointing to the left of 'p'
		// (the 180 degree edge)
		// we want to seach the edges opposite from the hint edge first
		const std::array<tile_edge, 8>* search_order = &detail::search_orders.front();
		auto reverse = false;

		// if hint was provided, then rotate the search order until hint
		// is on the left (180 degrees)
		if (hint)
		{
			const auto &hint_val = *hint;
			const auto hint_edge1 = tile_edge{ hint_val.position - p, hint_val.edge };
			const auto hint_edge2 = swap_cliff_facing(hint_edge1);

			for (const auto& pattern : detail::search_orders)
			{
				if (pattern[0] == hint_edge1 ||
					pattern[0] == hint_edge2)
				{
					search_order = &pattern;
					reverse = true;
					break;
				}
				else if (pattern[7] == hint_edge1 ||
					pattern[7] == hint_edge2)
				{
					search_order = &pattern;
					reverse = false;
					break;
				}
			}
		}

		auto passthrough = [&p, &f](tile_edge e) {
			e.position += p;
			return std::invoke(f, e);
			};

		// call 'f' with each edge(except the hint edge)
		if (reverse)
		{
			std::for_each(rbegin(*search_order), prev(rend(*search_order)), passthrough);
			// only call on the default hint edge if we didn't provide a hint
			if (!hint)
				std::invoke(f, search_order->front());
		}
		else
		{
			std::for_each(begin(*search_order), prev(end(*search_order)), passthrough);
			// only call on the default hint edge if we didn't provide a hint
			if (!hint)
				std::invoke(f, search_order->back());
		}
		return;
	}

	// calls Func for each cliff adjacent to p
	// Maximum two calls to Func for a valid map
	template<std::invocable<tile_edge> Func>
	static void for_each_adjacent_cliff(const terrain_map& m, const terrain_vertex_position p, Func &&f)
	{
		const auto size = get_size(m);
		for_each_safe_adjacent_corner(m, p, [&m, &f, size](const tile_position pos, const rect_corners corner) {
			if (!within_world(pos, size))
				return;

			const auto cliffs = get_cliff_info(pos, m);
			const auto tri_height = get_height_for_triangles(pos, m);

			const auto check_diag = [&](const rect_edges edge) {
				const auto diag_height = get_height_for_diag_edge(tri_height);
				const auto left = diag_height[0] > diag_height[2] || diag_height[1] > diag_height[3];
				std::invoke(f, tile_edge{ pos, edge, left });
			};

			const auto check_right = [&]() {
				const auto next_pos = pos + tile_position{ 1, 0 };
				const auto next_tile_height = get_height_for_triangles(next_pos, m);
				// right edge of pos
				const auto right_height = get_height_for_right_edge(tri_height);
				// left edge of tile to the right of pos
				const auto left_height = get_height_for_left_edge(next_tile_height);
				if (right_height[0] > left_height[0] || right_height[1] > left_height[1])
					std::invoke(f, tile_edge{ pos, rect_edges::right });
				else
					std::invoke(f, tile_edge{ next_pos, rect_edges::left });
			};

			const auto check_bottom = [&]() {
				const auto next_pos = pos + tile_position{ 0, 1 };
				const auto next_tile_height = get_height_for_triangles(next_pos, m);
				// bottom edge of pos
				const auto bottom_height = get_height_for_bottom_edge(tri_height);
				// top edge of tile below pos
				const auto top_height = get_height_for_top_edge(next_tile_height);
				if (bottom_height[0] > top_height[0] || bottom_height[1] > top_height[1])
					std::invoke(f, tile_edge{ pos, rect_edges::bottom });
				else
					std::invoke(f, tile_edge{ next_pos, rect_edges::top });
				};

			// TODO: the diag checks can be turned into a single check with diag_downhill etc.
			switch (corner)
			{
			case rect_corners::top_left:
			{
				if (cliffs.triangle_type == terrain_map::triangle_downhill &&
					cliffs.diag)
				{
					check_diag(rect_edges::downhill);
				}
			} break;
			case rect_corners::top_right:
			{
				if (cliffs.right)
					check_right();

				if (cliffs.triangle_type == terrain_map::triangle_uphill &&
					cliffs.diag)
				{
					check_diag(rect_edges::uphill);
				}
			} break;
			case rect_corners::bottom_left:
			{
				if (cliffs.bottom)
					check_bottom();

				if (cliffs.triangle_type == terrain_map::triangle_uphill &&
					cliffs.diag)
				{
					check_diag(rect_edges::uphill);
				}
			} break;
			case rect_corners::bottom_right:
			{
				if (cliffs.bottom)
					check_bottom();

				if (cliffs.right)
					check_right();

				if (cliffs.triangle_type == terrain_map::triangle_downhill &&
					cliffs.diag)
				{
					check_diag(rect_edges::downhill);
				}
			} break;
			}
			return;
			});

		return;
	}

	// NOTE: used in terrain.inl
	std::uint8_t detail::count_adjacent_cliffs(const terrain_map& m, const terrain_vertex_position p)
	{
		auto count = std::uint8_t{};
		for_each_adjacent_cliff(m, p, [&count](const tile_edge) noexcept {
			++count;
		});

		return count;
	}

	enum class adjacent_cliff_return : std::uint8_t {
		no_cliffs,
		begin = no_cliffs,
		single_cliff,
		too_many_cliffs,
		end
	};

	static std::tuple<adjacent_cliff_return, tile_edge> get_adjacent_cliff(const terrain_map& m, const terrain_vertex_position p)
	{
		using enum adjacent_cliff_return;
		auto too_many_cliffs_tag = false;
		auto edge = std::optional<tile_edge>{};
		
		for_each_adjacent_cliff(m, p, [&too_many_cliffs_tag, &edge](const tile_edge e) noexcept {
			if (edge)
				too_many_cliffs_tag = true;

			edge = e;
		});

		if (too_many_cliffs_tag)
			return { too_many_cliffs, {} };
		
		if (edge)
			return { single_cliff, *edge };

		return { no_cliffs, {} };
	}

	// returns a valid specific vertex for the passed vertex
	[[deprecated]]
	static detail::specific_vertex get_any_specific(const terrain_map& m, const terrain_vertex_position p)
	{
		const auto size = get_size(m);
		if (!within_world(p, size + tile_position{ 1, 1 }))
			throw terrain_error{ "vertex is outside the map" };

		const auto y_bottom = p.y > size.y;
		
		auto tile = p;
		auto corner = rect_corners::last;
		auto triangle = terrain_map::triangle_left;
	
		if (p.x == 0)
		{
			if (y_bottom)
			{
				tile -= tile_position{ 0, 1, };
				corner = rect_corners::bottom_left;
			}
			else
			{
				corner = rect_corners::top_left;
			}
		}
		else
		{
			tile -= tile_position{ 1, 0 };
			triangle = terrain_map::triangle_right;
			if (y_bottom)
			{
				tile -= tile_position{ 0, 1, };
				corner = rect_corners::bottom_right;
			}
			else
			{
				corner = rect_corners::top_right;
			}
		}

		return { tile, corner, triangle };
	}

	// returns true if all the specific vertex are the same height
	static bool is_flat_cliff(const terrain_map& m, const terrain_vertex_position p)
	{
		auto height = std::optional<std::uint8_t>{};
		auto ret = true;
		auto compare = [&height, &ret](const std::uint8_t h) noexcept {
			if (!height)
				height = h; 
			else if(*height != h)
				ret = false;
			return;
			};

		for_each_safe_adjacent_corner(m, p, [&m, &compare](const tile_position p, const rect_corners c) {
			const auto tris = get_height_for_triangles(p, m);
			const auto [first, second] = quad_index_to_triangle_index(c, tris.triangle_type);
			switch (c)
			{
			default:
				log_warning("Invalid corner value passed to functor from for_each_safe_adjacent_corner"sv);
				[[fallthrough]];
			case rect_corners::top_left:
				compare(tris.height[first]);
				if (tris.triangle_type == terrain_map::triangle_downhill)
					compare(tris.height[second]);
				break;
			case rect_corners::top_right:
				compare(tris.height[second]);
				if (tris.triangle_type == terrain_map::triangle_uphill)
					compare(tris.height[first]);
				break;
			case rect_corners::bottom_right:
				compare(tris.height[second]);
				if (tris.triangle_type == terrain_map::triangle_downhill)
					compare(tris.height[first]);
				break;
			case rect_corners::bottom_left:
				compare(tris.height[first]);
				if (tris.triangle_type == terrain_map::triangle_uphill)
					compare(tris.height[second]);
				break;
			}
			return;
			});
		return ret;
	}

	// returns true if this is the kind of cliff that should have the same height
	// at both sides; usually cliff ends, that don't also touch the world border
	static bool should_be_flat_cliff(const terrain_map& m, const terrain_vertex_position p)
	{
		const auto count = detail::count_adjacent_cliffs(m, p);
		switch (count)
		{
		case 0:
			return true;
		case 1:
		{
			const auto size = get_vertex_size(m);
			// a cliff won't be flat when it meets the worlds edge
			return !edge_of_map(size, p);
		}
		case 2:
			[[fallthrough]];
		default:
			return false;
		}
	}

	// return the two vertex that define the start and end of an edge
	// TODO: deprecate in favor of get_specific_vertex_from_edge?
	std::pair<terrain_vertex_position, terrain_vertex_position>
		get_edge_vertex(const tile_edge& edge) noexcept
	{
		const auto& p = edge.position;
		const auto& e = edge.edge;
		switch (e)
		{
		case rect_edges::top:
			return { p, p + tile_position{ 1, 0 } };
		case rect_edges::right:
			return { p + tile_position{ 1, 0 }, p + tile_position{ 1, 1 } };
		case rect_edges::bottom:
			return { p + tile_position{ 0, 1 }, p + tile_position{ 1, 1 } };
		case rect_edges::left:
			return { p, p + tile_position{ 0, 1 } };
		case rect_edges::uphill:
			return { p + tile_position{ 0, 1 }, p + tile_position{ 1, 0 } };
		case rect_edges::downhill:
			return { p, p + tile_position{1, 1} };
		}

		return { bad_tile_position, bad_tile_position };
	}

	static bool is_clockwise(const tile_edge f, const terrain_vertex_position v)
	{
		// Ditermine if tile_edge is clockwise or counter-clockwise
		// a cliff was pointing towards the 90 degree mark, it would be clockwise
		// if the high side of the cliff was on the left hand side.

		using enum rect_edges;

		constexpr auto clockwise = true;
		constexpr auto anticlockwise = false;

		// bottom right of v
		if (f.position == v)
		{
			switch (f.edge)
			{
			case top:
				return anticlockwise;
			case left:
				return clockwise;
			case downhill:
				return !f.left_triangle;
			default:
				throw logic_error{ "tile_edge doesn't connect with the provided vertex" };
			}
		}

		// bottom left of v
		if (f.position == v - tile_position{ 1, 0 })
		{
			switch (f.edge)
			{
			case top:
				return clockwise;
			case right:
				return anticlockwise;
			case uphill:
				return !f.left_triangle;
			default:
				throw logic_error{ "tile_edge doesn't connect with the provided vertex" };
			}
		}

		// top left of v
		if (f.position == v - tile_position{ 1, 1 })
		{
			switch (f.edge)
			{
			case bottom:
				return anticlockwise;
			case right:
				return clockwise;
			case downhill:
				return f.left_triangle;
			default:
				throw logic_error{ "tile_edge doesn't connect with the provided vertex" };
			}
		}

		// top right of v
		if (f.position == v - tile_position{ 0, 1 })
		{
			switch (f.edge)
			{
			case left:
				return anticlockwise;
			case bottom:
				return clockwise;
			case uphill:
				return f.left_triangle;
			default:
				throw logic_error{ "tile_edge doesn't connect with the provided vertex" };
			}
		}

		throw logic_error{ "tile_edge doesn't connect with the provided vertex" };
	}

	static bool compatible_cliff_edges(const tile_edge f, const tile_edge s, const terrain_vertex_position v)
	{
		return is_clockwise(f, v) != is_clockwise(s, v);
	}

	namespace detail
	{
		static constexpr bool is_cliff_impl(const terrain_map::cliff_info info, const rect_edges e) noexcept;

		struct can_add_cliff_return
		{
			bool already_exists : 1;
			bool f_is_world_edge : 1, s_is_world_edge : 1;
			adjacent_cliff_return f_cliff : 2;
			adjacent_cliff_return s_cliff : 2;
		};

		static std::optional<can_add_cliff_return> can_add_cliff_impl(const terrain_map& m, const tile_edge t_edge)
		{
			const auto normal_edge = normalise(t_edge);

			if (const auto w_size = get_size(m); !within_world(normal_edge.position, w_size))
				return {}; // outside the world

			const auto info = get_cliff_info(normal_edge.position, m);
			if (detail::is_cliff_impl(info, normal_edge.edge)) // Rule 6
				return can_add_cliff_return{ true };

			if ((t_edge.edge == rect_edges::downhill ||
				t_edge.edge == rect_edges::uphill) &&
				(info.diag_downhill || info.diag_uphill)) // Rule 5
			{
				return {};
			}

			const auto vert_size = get_vertex_size(m);
			// Each cliff is defined by two verticies
			const auto verts = get_edge_vertex(t_edge);

			const auto f_map_edge = edge_of_map(vert_size, verts.first);
			const auto s_map_edge = edge_of_map(vert_size, verts.second);

			// can't cliff along the edge of the world
			if (f_map_edge && s_map_edge) // Rule 3
				return {};

			const auto [f_cliff, f_edge] = get_adjacent_cliff(m, verts.first);
			const auto [s_cliff, s_edge] = get_adjacent_cliff(m, verts.second);

			using enum adjacent_cliff_return;

			if (s_cliff == single_cliff && // Rule 4
				!compatible_cliff_edges(t_edge, s_edge, verts.second))
				return {};

			if (f_cliff == single_cliff && // Rule 4
				!compatible_cliff_edges(t_edge, f_edge, verts.first))
				return {};

			// start a new cliff touching the worlds edge
			// only a single cliff can connect to a world edge vertex
			if (f_map_edge && f_cliff > no_cliffs) // Rule 1.a
				return {};
			if (s_map_edge && s_cliff > no_cliffs) // Rule 1.a
				return {};

			if (f_cliff == too_many_cliffs || // Rule 1
				s_cliff == too_many_cliffs)
				return {};

			assert(f_cliff < end);
			assert(s_cliff < end);
			return can_add_cliff_return{ false, f_map_edge, s_map_edge, f_cliff, s_cliff };
		}
	}

	// === Cliff Placement Rules ===
	//	1.	A vertex can only have max 2 adjacent cliff.
	//	1. a) A world edge vertex can only have 1 adjacent cliff.
	//	2.	Minimum cliff length is 2 edges.
	//	2. a) A cliff that touches the world edge is permitted to be only a single edge long.
	//  3.	Both verticies of a cliff cannot be world edge verticies
	//	4.	Two cliff edges that share a vertex must pass the compatible_cliff_edges test.
	//	5.	A tile cannot have more than one diagonal cliff edge (ie. an uphill and downhill cliff)
	//		even though those cliffs may not share any verticies.
	//	6.	A cliff can be placed over an identical cliff (nothing is changed).

	bool can_add_cliff(const terrain_map& m, const tile_edge t_edge)
	{
		// covers rules 1, 3, 4, 5 and 6
		const auto ret = detail::can_add_cliff_impl(m, t_edge);
		if (!ret)
			return false;

		if (ret->already_exists)
			return true;

		using enum adjacent_cliff_return;

		// at least one edge must already be a cliff // Rule 2 && 2.a
		if (!(ret->f_is_world_edge || ret->s_is_world_edge) &&
			ret->f_cliff == no_cliffs && ret->s_cliff == no_cliffs)
			return false;

		return true;
	}

	std::optional<tile_edge> can_start_cliff(const terrain_map& m, tile_edge e)
	{
		const auto can_add_e = detail::can_add_cliff_impl(m, e);
		if (!can_add_e)
			return {};
		// NOTE: since can_add_e is true, we must be compatible with any adjacent cliffs

		// we also need to find at least one other cliffable
		// edge to create the cliff with
		auto existing_edge = std::optional<tile_edge>{};
		auto out_edge = std::optional<tile_edge>{};
		terrain_vertex_position shared_vertex [[indeterminate]];
		const auto check_for_cliffs = [&existing_edge, &out_edge, &shared_vertex, &m, e](const auto edge) {
			if (existing_edge)
				return;
			const auto can_add_e = detail::can_add_cliff_impl(m, edge);
			if (!can_add_e)
				return;

			if (can_add_e->already_exists)
				existing_edge = edge;
			else if (!out_edge)
			{
				if (compatible_cliff_edges(edge, e, shared_vertex))
					out_edge = edge;
				else
					out_edge = swap_cliff_facing(edge);
			}
			return;
			};

		const auto [vert1, vert2] = get_edge_vertex(e);
		shared_vertex = vert1;
		for_each_adjacent_edge(vert1, e, check_for_cliffs);
		if (!existing_edge) // an existing edge is always prioritised over a compatible edge
		{
			shared_vertex = vert2;
			for_each_adjacent_edge(vert2, e, check_for_cliffs);
		}

		if (existing_edge)
			return existing_edge;
		return out_edge;
	}

	bool can_add_or_start_cliff(const terrain_map& m, const tile_edge e1, const tile_edge e2)
	{
		// undefined bahaviour if both params are the same edge
		assert(e1 != e2);
		assert(e1 != swap_cliff_facing(e2));

		// both edges must share a vertex
		const terrain_vertex_position shared_vert = [&]() noexcept {
			const auto [e1_v1, e1_v2] = get_edge_vertex(e1);
			const auto [e2_v1, e2_v2] = get_edge_vertex(e2);
			if (e1_v1 == e2_v1 ||
				e1_v1 == e2_v2)
				return e1_v1;
			if (e1_v2 == e2_v1 ||
				e1_v2 == e2_v2)
				return e1_v2;
			return bad_tile_position;
			}();

		// failed to find a shared vertex
		if (shared_vert == bad_tile_position)
			return false; // TODO: maybe throw here, this represents a major bug in the code that called this function

		if (!compatible_cliff_edges(e1, e2, shared_vert))
			return false;

		const auto map_size = get_vertex_size(m);
		const auto shared_edge = edge_of_map(map_size, shared_vert);

		// The edge can only be touched by a single cliff
		if (shared_edge)
			return false;

		const auto can_add_e1 = detail::can_add_cliff_impl(m, e1);
		const auto can_add_e2 = detail::can_add_cliff_impl(m, e2);

		return can_add_e1 && can_add_e2;
	}

	bool no_cliffs(const terrain_map& m, const tile_position p, const rect_edges e)
	{
		const auto verts = get_edge_vertex({ p, e });

		const auto f_count = detail::count_adjacent_cliffs(m, verts.first);
		const auto s_count = detail::count_adjacent_cliffs(m, verts.second);

		return f_count == 0 &&
			s_count == 0;
	}

	namespace detail
	{
		static constexpr bool is_cliff_impl(const terrain_map::cliff_info info, const rect_edges e) noexcept
		{
			switch (e)
			{
			case rect_edges::right:
				return info.right;
			case rect_edges::bottom:
				return info.bottom;
			case rect_edges::uphill: // TODO: we shouldn't need to second half of these checks
				return info.diag_uphill || (info.triangle_type == terrain_map::triangle_uphill && info.diag);
			case rect_edges::downhill:
				return info.diag_downhill || (info.triangle_type == terrain_map::triangle_downhill && info.diag);
			default:
				return false;
			}
		}
	}

	// TODO: make this static and deprecate the declaration in the header
	bool is_cliff(const tile_position t, const rect_edges e, const terrain_map& m)
	{
		const auto edge = normalise(tile_edge{ t, e });

		if (const auto w_size = get_size(m); !within_world(edge.position, w_size))
			return false;

		const auto info = get_cliff_info(edge.position, m);
		return detail::is_cliff_impl(info, edge.edge);
	}

	bool is_cliff(const tile_edge e, const terrain_map& m)
	{
		return is_cliff(e.position, e.edge, m);
	}

	// returns the two specific vertex defining the high end of the cliff
	static constexpr std::pair<detail::specific_vertex, detail::specific_vertex> get_specific_vertex_from_edge(const tile_edge e, const bool triangle_type) noexcept
	{
		using enum rect_edges;
		using enum rect_corners;

		static_assert(terrain_map::triangle_downhill == terrain_map::triangle_left);
		// NOTE: terrain_map::downhill == terrain_map::left_triangle
		//			so we can select the correct triangle with triangle_type or !triangle_type

		switch (e.edge)
		{
		case top:
			return {
				detail::specific_vertex{ e.position, top_left, !triangle_type },
				detail::specific_vertex{ e.position, top_right, !triangle_type }
			};
		case left:
			return {
				detail::specific_vertex{ e.position, top_left, terrain_map::triangle_left },
				detail::specific_vertex{ e.position, bottom_left, terrain_map::triangle_left }
			};
		case right:
			return {
				detail::specific_vertex{ e.position, top_right, terrain_map::triangle_right },
				detail::specific_vertex{ e.position, bottom_right, terrain_map::triangle_right }
			};
		case bottom:
			return {
				detail::specific_vertex{ e.position, bottom_left, triangle_type },
				detail::specific_vertex{ e.position, bottom_right, triangle_type }
			};
		case uphill:
			return {
				detail::specific_vertex{ e.position, bottom_left, e.left_triangle },
				detail::specific_vertex{ e.position, top_right, e.left_triangle }
			};
		case downhill:
			return {
				detail::specific_vertex{ e.position, top_left, e.left_triangle },
				detail::specific_vertex{ e.position, bottom_right, e.left_triangle }
			};
		default:
			assert(false);
		}
		
		return { detail::bad_specific_vertex, detail::bad_specific_vertex };
	}

	static void add_cliff_impl(const tile_edge e, std::uint8_t height_diff,
		terrain_map& m, const resources::terrain_settings& s)
	{
		// This function operates under the assumption that it is valid to 
		// add the cliff 'e'
		assert(detail::can_add_cliff_impl(m, e));

		const auto add_height = [height_diff](const std::uint8_t h) noexcept {
			const auto ret = h + height_diff;
			return integer_clamp_cast<std::uint8_t>(ret);
			};

		using enum rect_edges;
		// record the triangle type here so it can be used when applying the height change
		bool triangle_type [[indeterminate]];
		auto pos = e.position;
		// Tag the edge as a cliff and store the triangle type for when we
		// change the height
		switch (e.edge)
		{
		case top:
			pos -= tile_position{ 0, 1 };
			if (pos.y < 0)
				throw terrain_error{ "attempted to create a cliff outside the map borders"s };
			[[fallthrough]];
		case bottom:
		{
			const auto tile_above_index = to_tile_index(pos, m);
			auto cliff_inf = get_cliff_info(tile_above_index, m);
			cliff_inf.bottom = true;
			set_cliff_info(tile_above_index, m, cliff_inf);
			triangle_type = cliff_inf.triangle_type;
		} break;
		case left:
			pos -= tile_position{ 1, 0 }; 
			if (pos.x < 0)
				throw terrain_error{ "attempted to create a cliff outside the map borders"s };
			[[fallthrough]];
		case right:
		{
			const auto tile_index = to_tile_index(pos, m);
			auto cliff_inf = get_cliff_info(tile_index, m);
			cliff_inf.right = true;
			set_cliff_info(tile_index, m, cliff_inf);
			triangle_type = cliff_inf.triangle_type;
		} break;
		default:
			log_warning("Unexpected edge type in add_cliff_impl, defaulting to uphill"s);
			//TODO: just throw logic error here
			[[fallthrough]];
		case uphill:
		{
			const auto tile_index = to_tile_index(e.position, m);
			auto cliff_inf = get_cliff_info(tile_index, m);
			if (cliff_inf.triangle_type != terrain_map::triangle_uphill)
			{
				swap_triangle_type(m, e.position);
				cliff_inf = get_cliff_info(tile_index, m);
			}
			cliff_inf.diag = cliff_inf.diag_uphill = true;
			set_cliff_info(tile_index, m, cliff_inf);
			triangle_type = terrain_map::triangle_uphill;
		} break;
		case downhill:
		{
			const auto tile_index = to_tile_index(e.position, m);
			auto cliff_inf = get_cliff_info(tile_index, m);
			if (cliff_inf.triangle_type != terrain_map::triangle_downhill)
			{
				swap_triangle_type(m, e.position);
				cliff_inf = get_cliff_info(tile_index, m);
			}
			cliff_inf.diag = cliff_inf.diag_downhill = true;
			set_cliff_info(tile_index, m, cliff_inf);
			triangle_type = terrain_map::triangle_downhill;
		}
		}

		const auto specific_verts = get_specific_vertex_from_edge(e, triangle_type);

		const auto vertex1 = to_vertex_position(specific_verts.first);
		const auto vertex2 = to_vertex_position(specific_verts.second);

		const auto v1_is_flat = is_flat_cliff(m, vertex1);
		const auto v1_should_be_flat = should_be_flat_cliff(m, vertex1);

		if (v1_is_flat && !v1_should_be_flat)
		{
			const auto& [tile_pos, corner, tri] = specific_verts.first;
			change_terrain_height(tile_pos, corner, tri, m, s, add_height);
		}

		const auto v2_is_flat = is_flat_cliff(m, vertex2);
		const auto v2_should_be_flat = should_be_flat_cliff(m, vertex2);

		if (v2_is_flat && !v2_should_be_flat)
		{
			const auto& [tile_pos, corner, tri] = specific_verts.second;
			change_terrain_height(tile_pos, corner, tri, m, s, add_height);
		}

		return;
	}

	void add_cliff(const tile_edge e, const std::uint8_t h, terrain_map& m, const resources::terrain_settings& s)
	{
		assert(can_add_cliff(m, e));
		add_cliff_impl(e, h, m, s);
		return;
	}

	void add_cliff(const tile_edge e1, const tile_edge e2, const std::uint8_t add_height, terrain_map& m, const resources::terrain_settings& s)
	{
		assert(can_add_or_start_cliff(m, e1, e2));
		add_cliff_impl(e1, add_height, m, s);
		add_cliff_impl(e2, add_height, m, s);
		return;
	}

	namespace
	{
		// TODO: might be time to add a triangle_math header
		// 
		// Based on PointInTriangle from: https://stackoverflow.com/a/20861130
		constexpr bool point_in_tri(const vector2_float p, const vector2_float p0, const vector2_float p1, const vector2_float p2) noexcept
		{
			const auto s = (p0.x - p2.x) * (p.y - p2.y) - (p0.y - p2.y) * (p.x - p2.x);
			const auto t = (p1.x - p0.x) * (p.y - p0.y) - (p1.y - p0.y) * (p.x - p0.x);

			if ((s < 0) != (t < 0) && s != 0 && t != 0)
				return false;

			const auto d = (p2.x - p1.x) * (p.y - p1.y) - (p2.y - p1.y) * (p.x - p1.x);
			return d == 0 || (d < 0) == (s + t <= 0);
		}

		constexpr bool point_in_tri(const vector2_float p, const std::array<vector2_float, 3>& tri) noexcept
		{
			return point_in_tri(p, tri[0], tri[1], tri[2]);
		}

		struct barycentric_point
		{
			float u, v, w;
		};

		constexpr bool operator==(const barycentric_point& lhs, const barycentric_point& rhs) noexcept
		{
			return std::tie(lhs.u, lhs.v, lhs.w) == std::tie(rhs.u, rhs.v, rhs.w);
		}

		constexpr barycentric_point to_barycentric(const vector2_float p, const vector2_float p1, const vector2_float p2, const vector2_float p3) noexcept
		{
			const auto t = (p2.y - p3.y) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.y - p3.y);
			const auto u = ((p2.y - p3.y) * (p.x - p3.x) + (p3.x - p2.x) * (p.y - p3.y)) / t;
			const auto v = ((p3.y - p1.y) * (p.x - p3.x) + (p1.x - p3.x) * (p.y - p3.y)) / t;
			const auto w = 1.f - u - v;
			return { u, v, w };
		}

		constexpr barycentric_point to_barycentric(const vector2_float p, const std::array<vector2_float, 3> &tri) noexcept
		{
			return to_barycentric(p, tri[0], tri[1], tri[2]);
		}

		constexpr vector2_float from_barycentric(const barycentric_point b, const vector2_float p1, const vector2_float p2, const vector2_float p3) noexcept
		{
			const auto& [u, v, w] = b;
			return { u * p1.x + v * p2.x + w * p3.x, u * p1.y + v * p2.y + w * p3.y };
		}

		constexpr vector2_float from_barycentric(const barycentric_point b, const std::array<vector2_float, 3> &tri) noexcept
		{
			return from_barycentric(b, tri[0], tri[1], tri[2]);
		}

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
			const vector2_float height_dir, const triangle_height_data& heightmap) noexcept
		{
			tris out [[indeterminate]];
			if (heightmap.triangle_type == terrain_map::triangle_uphill)
			{
				out.flat_left_tri = { make_quad_point(pos, tile_sizef, rect_corners::top_left),
					make_quad_point(pos, tile_sizef, rect_corners::bottom_left),
					make_quad_point(pos, tile_sizef, rect_corners::top_right) };
				out.flat_right_tri = { out.flat_left_tri[2], out.flat_left_tri[1],
					make_quad_point(pos, tile_sizef, rect_corners::bottom_right) };
			}
			else
			{
				out.flat_left_tri = { make_quad_point(pos, tile_sizef, rect_corners::top_left),
					make_quad_point(pos, tile_sizef, rect_corners::bottom_left),
					make_quad_point(pos, tile_sizef, rect_corners::bottom_right) };
				out.flat_right_tri = { out.flat_left_tri[0], out.flat_left_tri[2],
					make_quad_point(pos, tile_sizef, rect_corners::top_right) };
			}

			const auto begin = std::begin(heightmap.height);
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

	// project 'p' onto the flat version of 'map'
	vector2_float project_onto_terrain(const vector2_float p, const float rot,
		const resources::tile_size_t tile_size, const terrain_map& map)
	{
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
			// TODO: check cliffs for each tile as well,
			//			- will require getting height from neighbouring tiles
			//			- will need to replace left_hit/right_hit with an array of tris
			//			- absent cliffs can just be degenerate triangles
			
			// get index for accessing heightmap
			const auto height_info = get_height_for_triangles(tile_check, map);
			// generate quad vertex
			const auto quad_triangles = get_quad_triangles(tile_check, tile_sizef, height_dir, height_info);

			// test for hit
			// NOTE: be mindful of how quads are triangled in terrain_map/animation.hpp
			// NOTE: this is the canonical vertex order for triangles in the terrain system
			auto left_hit = point_in_tri(p, quad_triangles.left_tri),
				right_hit = point_in_tri(p, quad_triangles.right_tri);

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
				bary_point = to_barycentric(p, quad_triangles.left_tri);
				last_tri = quad_triangles.flat_left_tri;
			}
			else if (right_hit)
			{
				bary_point = to_barycentric(p, quad_triangles.right_tri);
				last_tri = quad_triangles.flat_right_tri;
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
			return p;
		// calculate pos within the hit tri
		return from_barycentric(bary_point, last_tri);
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

	static void load_terrainset(terrainset& terr, data::data_manager &d)
	{
		for (const auto& t : terr.terrains)
				d.get<terrain>(t->id);

		terr.loaded = true;
		return;
	}

	void terrainset::load(data::data_manager& d)
	{
		load_terrainset(*this, d);
		return;
	}

	void terrainset::serialise(const data::data_manager& d, data::writer& w) const
	{
		//terrainsets:
		//	name: [terrains, terrains, terrains] // not a merable sequence, this will overwrite

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
		if (s.editor_terrain)
		{
			d.get<terrain>(s.editor_terrain.id());
			// replace editor terrainset with the editor terrain
			auto tset = d.get<terrainset>(s.editor_terrainset.id());
			tset->terrains.clear();
			tset->terrains.emplace_back(d.make_resource_link<terrain>(s.editor_terrain.id(), s.editor_terrainset.id()));
		}

		if (s.empty_terrain)
			d.get<terrain>(s.empty_terrain.id());

		for (const auto& t : s.terrains)
			d.get<terrain>(t.id());

		for (const auto& t : s.terrainsets)
			d.get<terrainset>(t.id());

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

		if(height_min != std::numeric_limits<std::uint8_t>::min())
			w.write("height-min"sv, height_min);
		if(height_max != std::numeric_limits<std::uint8_t>::max())
			w.write("height-max"sv, height_max);
		if (height_default != std::uint8_t{})
			w.write("height-default"sv, height_default);

		if (editor_terrain)
			w.write("editor-terrain"sv, d.get_as_string(editor_terrain.id()));

		return;
	}

	// if anyone asks for the 'none' type just give them the empty tile
	template<class U = std::vector<tile>, class W>
	U& get_transition(transition_tile_type type, W& t, const resources::terrain_settings& s)
	{
		// is_const is only false for the functions inside this file
		// where we want to assert
		if constexpr (std::is_const_v<U>)
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

			auto &transition_vector = get_transition(t, terrain, s);

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

		for (const auto &str : s)
		{
			assert(std::numeric_limits<uint8>::max() > std::size(transition_names));
			assert(std::size(transition_names) == std::size(transition_short_names));
			for (auto i = uint8{}; i < std::size(transition_names); ++i)
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
			//copy the range until it is longer than tile_count
			while (count > std::size(transitions))
				std::copy(std::begin(transitions), std::end(transitions), std::back_inserter(transitions));

			//trim the vector back to tile_count length
			assert(count < std::size(transitions));
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

			t->cliff_terrain = data::parse_tools::get_scalar(*terrainset_n, "cliff"sv, t->cliff_terrain, [id, &d](std::string_view s) {
				const auto i = d.get_uid(s);
				return d.make_resource_link<terrain>(i, id);
			});

			settings->terrainsets.emplace_back(d.make_resource_link<terrainset>(id, resources::get_tile_settings_id()));
		}

		remove_duplicates(settings->terrainsets);
	}

	static void parse_terrain_settings(unique_id mod, const data::parser_node& n, data::data_manager& d)
	{
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

        //const auto id = d.get_uid(resources::get_tile_settings_name());
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

		return;
	}

	const terrain_settings *get_terrain_settings()
	{
		return data::get<terrain_settings>(id::terrain_settings);
	}

	// TODO: constexpr
	std::string_view get_empty_terrainset_name() noexcept
	{
		using namespace::std::string_view_literals;
		return "air-terrainset"sv;
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
		return get_transition(ty, t, s);
	}

	const std::vector<tile>& get_transitions(const terrain &t, transition_tile_type ty, const resources::terrain_settings& s)
	{
		return get_transition<const std::vector<tile>>(ty, t, s);
	}

	tile get_random_tile(const terrain &t, transition_tile_type type, const resources::terrain_settings& s)
	{
		const auto& vec = get_transitions(t, type, s);
		assert(!empty(vec));
		return *random_element(std::begin(vec), std::end(vec));
	}
}
