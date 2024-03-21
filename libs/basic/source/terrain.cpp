#include "hades/terrain.hpp"

#include <map>
#include <span>

#include "hades/data.hpp"
#include "hades/deflate.hpp"
#include "hades/parser.hpp"
#include "hades/random.hpp"
#include "hades/table.hpp"
#include "hades/tiles.hpp"
#include "hades/writer.hpp"

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

	using namespace std::string_view_literals;

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
		using namespace std::string_view_literals;
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

		throw terrain_error{"tried to index a terrain that isn't in this terrain set"};
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
			LOGWARNING("tile information must exist for each tile");
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

	using namespace std::string_view_literals;
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

		using namespace std::string_view_literals;
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

	std::array<terrain_index_t, 4> to_terrain_index(const tile_position tile_index, const tile_index_t terrain_width) noexcept
	{
		const auto index = to_tile_index(tile_index, terrain_width);
		return { index, index + 1, index + 1 + terrain_width, index + terrain_width };
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
		const auto& c = get_cliff_info(p, m);
		return triangle_height_data{
			std::array<std::uint8_t, 6>{
				h[0], h[1], h[2], h[3] ,h[4], h[5],
		}, c.triangle_type
		};
	}

	void set_height_for_triangles(const tile_position p, terrain_map& m, const triangle_height_data t)
	{
		const auto index = to_tile_index(p, m);
		auto h = detail::get_triangle_height(index, m);
		h[0] = t.height[0];
		h[1] = t.height[1];
		h[2] = t.height[2];
		h[3] = t.height[3];
		h[4] = t.height[4];
		h[5] = t.height[5];
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
		template<std::invocable<std::uint8_t, std::uint8_t> Func>
		std::uint8_t read_triangles_as_quad(const triangle_height_data& t, rect_corners c, Func&& f) 
						noexcept(std::is_nothrow_invocable_v<Func, std::uint8_t, std::uint8_t>)
		{
			const auto [first, second] = quad_index_to_triangle_index(c, t.triangle_type);
			if (second != bad_triangle_index)
				return std::invoke(f, t.height[first], t.height[second]);

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

	static void set_cliff_info(const tile_index_t ind, terrain_map& m, const terrain_map::cliff_info c) noexcept
	{
		assert(ind < size(m.cliff_data));
		m.cliff_data[ind] = c;
		m.cliffs = generate_cliff_layer(m, *resources::get_terrain_settings());
		return;
	}

	terrain_map::cliff_info get_cliff_info(const tile_position p, const terrain_map& m) noexcept
	{
		const auto ind = to_tile_index(p, m);
		return get_cliff_info(ind, m);
	}

	void set_cliff_info_tmp(tile_position p, terrain_map& m, terrain_map::cliff_info c) // TODO: temp remove
	{
		const auto ind = to_tile_index(p, m);
		return set_cliff_info(ind, m, c);
	}

	std::array<bool, 4> get_adjacent_cliffs(const tile_position p, const terrain_map& m) noexcept
	{
		const auto find_cliffs = [](const tile_position p, const terrain_map& m) noexcept {
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
		};
	}

	std::array<bool, 4> get_cliffs_corners(const tile_position p, const terrain_map& m) noexcept
	{
		// TODO: this needs to include checks against adjacent tiles, 
		//			perhaps an extra function needs to do 

		// see for_each_tile_corner

		const auto find_cliffs = [](const tile_position p, const terrain_map& m) noexcept {
			if (p.x < 0 || p.y < 0)
				return empty_cliff;

			return get_cliff_info(p, m);
			};

		const auto our_cliffs = get_cliff_info(p, m);
		const auto above = find_cliffs(p - tile_position{ 0, 1 }, m);
		const auto left = find_cliffs(p - tile_position{ 1, 0 }, m);

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

	bool is_tile_split(tile_position p, const terrain_map& m)
	{
		const auto& cliff = get_cliff_info(p, m);
		return cliff.diag;
	}

	const resources::terrain *get_vertex(const terrain_map &m, terrain_vertex_position p)
	{
		const auto index = to_1d_index(p, get_width(m));
        return m.terrain_vertex[integer_cast<std::size_t>(index)];
	}

	tag_list get_tags_at(const terrain_map &m, tile_position p)
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

	void resize_map_relative(terrain_map& m, vector2_int top_left, vector2_int bottom_right,
		const resources::terrain* t, std::uint8_t height, const resources::terrain_settings& s)
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

	void resize_map_relative(terrain_map& m, vector2_int top_left, vector2_int bottom_right)
	{
		const auto settings = resources::get_terrain_settings();
		assert(settings);
		const auto terrain = settings->empty_terrain.get();
		resize_map_relative(m, top_left, bottom_right, terrain, settings->height_default, *settings);
	}

	void resize_map(terrain_map& m, vector2_int s, vector2_int o,
		const resources::terrain* t, std::uint8_t height, const resources::terrain_settings& settings)
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

	void resize_map(terrain_map& m, vector2_int size, vector2_int offset)
	{
		const auto settings = resources::get_terrain_settings();
		assert(settings);
		const auto terrain = settings->empty_terrain.get();
		resize_map(m, size, offset, terrain, settings->height_default, *settings);
	}

	[[deprecated]] std::vector<tile_position> get_adjacent_tiles(terrain_vertex_position p)
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

	[[deprecated]] std::vector<tile_position> get_adjacent_tiles(const std::vector<terrain_vertex_position> &v)
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
	void place_tile(terrain_map &m, tile_position p, const resources::tile &t, const resources::terrain_settings& s)
	{
		place_tile(m.tile_layer, p, t, s);
	}

	void place_tile(terrain_map &m, const std::vector<tile_position> &p, const resources::tile &t, const resources::terrain_settings& s)
	{
		place_tile(m.tile_layer, p, t, s);
	}

	static void place_terrain_internal(terrain_map &m, terrain_vertex_position p, const resources::terrain *t)
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
		const auto t_p = get_adjacent_tiles(p);
		update_tile_layers_internal(m, t_p, s);
	}

	void place_terrain(terrain_map &m, terrain_vertex_position p, const resources::terrain *t,
		const resources::terrain_settings& s)
	{
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
		const auto s = get_vertex_size(m);
		for (const auto& p : positions)
			if (within_map(s, p))
				place_terrain_internal(m, p, t);

		update_tile_layers(m, positions, settings);
	}

	void raise_terrain(terrain_map& m, const terrain_vertex_position p, const std::uint8_t amount, const resources::terrain_settings* ts)
	{
		assert(ts);
		change_terrain_height(m, p, [amount, ts](const std::uint8_t h) noexcept {
			const auto compound = integer_cast<std::int16_t>(h) + integer_cast<std::int16_t>(amount);
			return std::clamp(integer_clamp_cast<std::uint8_t>(compound), ts->height_min, ts->height_max);
			});
		return;
	}

	void lower_terrain(terrain_map& m, const terrain_vertex_position p, const std::uint8_t amount, const resources::terrain_settings* ts)
	{
		assert(ts);
		change_terrain_height(m, p, [amount, ts](const std::uint8_t h) noexcept {
			const auto compound = integer_cast<std::int16_t>(h) - integer_cast<std::int16_t>(amount);
			return std::clamp(integer_clamp_cast<std::uint8_t>(compound), ts->height_min, ts->height_max);
			});
		return;
	}

	void set_height_at(terrain_map& m, const terrain_vertex_position p, std::uint8_t h, const resources::terrain_settings* ts)
	{
		assert(ts); 
		h = std::clamp(h, ts->height_min, ts->height_max);
		change_terrain_height(m, p, [h](const std::uint8_t) noexcept {
			return h;
			});
		return;
	}

	void swap_triangle_type(terrain_map& m, const tile_position p)
	{
		const auto index = to_tile_index(p, m);
		auto h = detail::get_triangle_height(index, m);
		auto c = get_cliff_info(p, m);
		// we should never do this if a tile contains a cliff
		assert(!c.diag);
		c.triangle_type = !c.triangle_type;
		if (c.triangle_type == terrain_map::triangle_uphill)
		{
			assert(h[0] == h[3] &&
				h[2] == h[4]);

			h[4] = h[1];
			h[3] = h[5];
			std::swap(h[5], h[2]);
		}
		else
		{
			assert(h[2] == h[3] &&
				h[1] == h[4]);

			h[3] = h[0];
			h[4] = h[5];
			std::swap(h[5], h[2]);
		}
		
		set_cliff_info(index, m, c);
		return;
	}

	// TODO: maybe move this into terrain.hpp
	//		and adopt the safe/unsafe impl style
	template<std::invocable<tile_position, rect_corners> Func>
	static void for_each_safe_adjacent_corner(const terrain_map& m, const terrain_vertex_position p, Func&& f) 
		noexcept(std::is_nothrow_invocable_v<Func, tile_position, rect_corners>)
	{
		constexpr auto corners = std::array{
			rect_corners::top_left,
			rect_corners::top_right,
			rect_corners::bottom_right,
			rect_corners::bottom_left
		};

		const auto positions = std::array{
			p,
			p - tile_position{ 1, 0 },
			p - tile_position{ 1, 1 },
			p - tile_position{ 0, 1 }
		};

		const auto world_size = get_size(m);
		constexpr auto size = std::size(corners);

		for (auto i = std::uint8_t{}; i != size; ++i)
		{
			if (within_map(world_size, positions[i]))
				std::invoke(f, positions[i], corners[i]);
		}

		return;
	}

	static std::uint8_t count_adjacent_cliffs(const terrain_map& m, const terrain_vertex_position p) noexcept
	{
		auto count = std::uint8_t{};
		for_each_safe_adjacent_corner(m, p, [&count, &m](const tile_position pos, const auto corner) noexcept {
			const auto cliffs = get_cliff_info(pos, m);
			switch (corner)
			{
			case rect_corners::top_left:
			{
				if (cliffs.triangle_type == terrain_map::triangle_downhill &&
					cliffs.diag)
					++count;
			} break;
			case rect_corners::top_right:
			{
				if (cliffs.right)
					++count;
				if (cliffs.triangle_type == terrain_map::triangle_uphill &&
					cliffs.diag)
					++count;
			} break;
			case rect_corners::bottom_left:
			{
				if (cliffs.bottom)
					++count;
				if (cliffs.triangle_type == terrain_map::triangle_uphill &&
					cliffs.diag)
					++count;
			} break;
			case rect_corners::bottom_right:
			{
				if (cliffs.bottom)
					++count;
				if (cliffs.right)
					++count;
				if (cliffs.triangle_type == terrain_map::triangle_downhill &&
					cliffs.diag)
					++count;
			} break;
			}
			return;
		});

		return count;
	}

	// return the two vertex that define the start and end of an edge
	static constexpr std::pair<terrain_vertex_position, terrain_vertex_position>
		get_edge_vertex(const tile_position p, const rect_edges e) noexcept
	{
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

	bool can_add_cliff(const terrain_map& m, const tile_position p, const rect_edges e)
	{
		const auto vert_size = get_vertex_size(m);
		// cliffs must exist in lines
		// only two cliff edges are allowed to touch a vertex
		const auto verts = get_edge_vertex(p, e);
		
		const auto f_edge = edge_of_map(vert_size, verts.first);
		const auto s_edge = edge_of_map(vert_size, verts.second);

		const auto f_count = count_adjacent_cliffs(m, verts.first);
		const auto s_count = count_adjacent_cliffs(m, verts.second);

		// can't cliff along the edge of the world
		if (f_edge && s_edge)
			return false; 
		
		// start a new cliff touching the worlds edge
		if (f_edge && s_count == 0)
			return true;
		if (s_edge && f_count == 0)
			return true;

		// add to end of existing cliff
		if (f_count == 0 &&
			s_count == 1)
			return true;
		if(s_count == 0 &&
			f_count == 1)
			return true;
		
		return false;
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
			tris out; // uninitialized
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
	vector2_float project_onto_terrain(const vector2_float p, float rot,
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

	//short transition names, same values an order as the previous list
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
