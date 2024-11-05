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

[[deprecated]]
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
			log_warning("missing vertex width"sv);

		//check the the map has a valid terrain vertex
		const auto size = get_size(r);
		const auto terrain_size = terrain_vertex_position{
            size.x + 1,
            size.y + 1
		};

		const auto vertex_length = integer_cast<std::size_t>(terrain_size.x) * integer_cast<std::size_t>(terrain_size.y);

		if (!std::empty(r.terrain_vertex) &&
			integer_cast<decltype(vertex_length)>(std::size(r.terrain_vertex)) != vertex_length)
		{
			LOGERROR("raw map terrain vertex list should be the same number of tiles as the tile_layer, or it should be empty."sv);
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
			// NOTE: this is only a warning
			// we can still try to load the map
			log_warning("Cliff layer must have a sample for each tile"sv);
		}

		if (std::size(r.ramp_layer) != tile_count)
		{
			// NOTE: this is only a warning
			// we can still try to load the map
			log_warning("Ramp layer must have a sample for each tile"sv);
		}

		if (!std::empty(r.terrain_layers))
		{
			if (std::any_of(std::begin(r.terrain_layers), std::end(r.terrain_layers), [size](auto &&l) {
				return get_size(l) != size;
			}))
			{
				LOGERROR("raw terrain map, if tile data for terrain layers is present, then those layers must be the same size as the tile-layer");
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
	constexpr auto terrain_cliff_layer_str = "cliff-layer"sv;
	constexpr auto terrain_layers_str = "terrain-layers"sv;
	constexpr auto terrain_vertex_width_str = "width"sv;

	void write_raw_terrain_map(const raw_terrain_map & m, data::writer & w)
	{
		// we start in a terrain: node

		//terrain:
		//	terrainset:
		//	terrain_vertex:
		//	vertex_height:
		//	terrain_layers:
		//  cliff_layers:
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

		// write terrain tile layers
		w.start_sequence(terrain_layers_str); 
		for (const auto &l : m.terrain_layers)
		{
			w.start_map();
			write_raw_map(l, w);
			w.end_map();
		}
		w.end_sequence();

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
		//	terrain_layers:
		//  cliff_layers:
		//	width:

		raw_terrain_map out [[indeterminate]];

		out.terrainset = data::parse_tools::get_unique(p, terrainset_str, unique_zero);

		detail::parse_compressed_sequence(p, terrain_vertex_str, out.terrain_vertex, vert_size);
		detail::parse_compressed_sequence(p, terrain_height_str, out.heightmap, vert_size);
		detail::parse_compressed_sequence(p, terrain_cliff_layer_str, out.cliff_layer, layer_size);

		auto layers = std::vector<raw_map>{};
		const auto layer_node = p.get_child(terrain_layers_str);
		for (const auto& l : layer_node->get_children())
			layers.emplace_back(read_raw_map(*l, layer_size));
		out.terrain_layers = std::move(layers);

		out.width = data::parse_tools::get_scalar<terrain_index_t>(p, terrain_vertex_width_str, bad_tile_index);

		return out;
	}

	// generate tiles for the cliffs
	[[deprecated]]
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
			if (false)
			{
				const auto& tile = resources::get_random_tile(cliff_terrain, resources::transition_tile_type::all, s);
				place_tile(layer, index + enum_type(tile_type::diag), tile, s);
			}
			// right
			if (false)
			{
				const auto& tile = resources::get_random_tile(cliff_terrain, resources::transition_tile_type::all, s);
				place_tile(layer, index + enum_type(tile_type::right), tile, s);
			}
			// bottom
			if (false)
			{
				const auto& tile = resources::get_random_tile(cliff_terrain, resources::transition_tile_type::all, s);
				place_tile(layer, index + enum_type(tile_type::bottom), tile, s);
			}
		}

		return layer;
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
		m.cliff_data = std::forward<Raw>(r).cliff_data;

		// TODO: heightmap and cliff_layer should be requied to be valid

		//tile layer is required to be valid
		m.tile_layer = to_tile_map(r.tile_layer);
		const auto size = get_size(m.tile_layer);

		// this block is deprecated
		if(std::empty(r.cliffs.tiles))
			m.cliffs = generate_cliff_layer(m, settings);
		else
			m.cliffs = to_tile_map(r.cliffs);

		// TODO:
		/*if(std::empty(r.cliff_layer))
			m.cliff_layer = */ // TODO:

		using ramp_layer_t = terrain_map::ramp_layer_t;
		if (std::empty(m.ramp_layer))
			m.ramp_layer = std::vector<ramp_layer_t>(std::size(m.cliff_layer), ramp_layer_t{}); // no ramps

		
		const auto empty = settings.empty_terrain.get();
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
		m.terrain_layers = generate_terrain_layers(m.terrainset, m.terrain_vertex, size.x, settings);

		if (std::empty(r.terrain_layers))
		{
			if (m.terrainset == settings.empty_terrainset.get())
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

		m.width = r.width;

		// TODO: this is a very expensive assert
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
		m.cliff_data = std::forward<Map>(t).cliff_data;

		//build a replacement lookup table
		auto t_map = std::map<const resources::terrain*, terrain_id_t>{};

		const auto empty = resources::get_empty_terrain(s);
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
		const auto vertex_size = integer_cast<std::size_t>((size.x + 1) * (size.y + 1));
		map.width = size.x + 1;
        map.terrain_vertex.resize(vertex_size, t);
		const auto tile_count = integer_cast<std::size_t>(size.x) * integer_cast<std::size_t>(size.y);
		map.heightmap.resize(vertex_size, s.height_default);
		map.cliff_layer.resize(tile_count, s.cliff_default);
		map.ramp_layer.resize(tile_count);
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

	void copy_heightmap(terrain_map& target, const terrain_map& src)
	{
		if (size(target.heightmap) != size(src.heightmap))
			throw terrain_error{ "Tried to copy heightmaps between incompatible maps" };
		target.heightmap = src.heightmap;
		return;
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

	triangle_height_data get_height_for_triangles(const tile_position p, const terrain_map& m, const resources::terrain_settings& s)
	{
		const auto tile_index = to_tile_index(p, m);
		assert(s.cliff_height > 0);
		
		const auto cliff_height = m.cliff_layer[tile_index] * s.cliff_height;

		const auto index = to_vertex_index(p, m);
		const auto tl = integer_clamp_cast<std::uint8_t>(m.heightmap[index] + cliff_height);
		const auto tr = integer_clamp_cast<std::uint8_t>(m.heightmap[integer_cast<std::size_t>(index) + 1] + cliff_height);
		const auto index2 = index + m.width;
		const auto bl = integer_clamp_cast<std::uint8_t>(m.heightmap[index2] + cliff_height);
		const auto br = integer_clamp_cast<std::uint8_t>(m.heightmap[integer_cast<std::size_t>(index2) + 1] + cliff_height);
		
		const auto ramps = get_adjacent_ramps(p, m);
		const auto cliff_layer_half = s.cliff_height / 2;

		const auto top_ramp_type = ramps[enum_type(rect_edges::top)];
		const auto right_ramp_type = ramps[enum_type(rect_edges::right)];
		const auto bottom_ramp_type = ramps[enum_type(rect_edges::bottom)];
		const auto left_ramp_type = ramps[enum_type(rect_edges::left)];

		const auto update_corner = [cliff_layer_half](const ramp_type a, const ramp_type b, const std::uint8_t corner_height) {
			const auto merged_ramp = merge_ramps(a, b);
			if (merged_ramp == ramp_type::uphill)
				return integer_clamp_cast<std::uint8_t>(corner_height + cliff_layer_half);
			else if (merged_ramp == ramp_type::downhill)
				return integer_clamp_cast<std::uint8_t>(corner_height - cliff_layer_half);

			return corner_height;
		};

		// TOP_LEFT corner
		const auto final_tl = update_corner(top_ramp_type, left_ramp_type, tl);
		// TOP_RIGHT corner
		const auto final_tr = update_corner(top_ramp_type, right_ramp_type, tr);
		// BOTTOM_RIGHT corner
		const auto final_br = update_corner(bottom_ramp_type, right_ramp_type, br);
		// BOTTOM_LEFT corner
		const auto final_bl = update_corner(bottom_ramp_type, left_ramp_type, bl);

		triangle_height_data ret [[indeterminate]];
		// interchange triangle_type along each axis
		if (p.y % 2 == 0)
			ret.triangle_type = terrain_map::triangle_type{ p.x % 2 == 0 };
		else
			ret.triangle_type = terrain_map::triangle_type{ p.x % 2 != 0 };

		if (ret.triangle_type == terrain_map::triangle_uphill)
		{
			ret.height = {
				final_tl, final_bl, final_tr,
				final_tr, final_bl, final_br
			};
		}
		else
		{
			ret.height = {
				final_tl, final_bl, final_br,
				final_tl, final_br, final_tr
			};
		}
		return ret;
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

	terrain_map::cliff_info get_cliff_info(const tile_index_t ind, const terrain_map& m) noexcept
	{
		assert(ind < size(m.cliff_data));
		return m.cliff_data[ind];
	}

	terrain_map::cliff_info get_cliff_info(const tile_position p, const terrain_map& m)
	{
		const auto ind = to_tile_index(p, m);
		return get_cliff_info(ind, m);
	}

	bool edge_of_map(const terrain_vertex_position map_size, const terrain_vertex_position position) noexcept
	{
		return position.x == 0 ||
			position.y == 0 ||
			position.x == map_size.x ||
			position.y == map_size.y;
	}
	
	std::array<std::uint8_t, 4> get_max_height_in_corners(const tile_position tile_index,
		const terrain_map& map, const resources::terrain_settings& settings)
	{
		const auto tris = get_height_for_triangles(tile_index, map, settings);
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

	[[deprected]]
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

	std::array<bool, 4> get_cliffs_corners(const tile_position p, const terrain_map& m)
	{
		// TODO: this needs to include checks against adjacent tiles, 
		//			perhaps an extra function needs to do 

		// see for_each_tile_corner

		assert(false);
		return {};
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

		template<bool CheckRampFlag = false>
		static std::array<ramp_type, 4> check_ramp_impl(const tile_position p, const terrain_map& m)
		{
			const auto world_size = get_size(m);
			assert(within_world(p, world_size));
			const auto start_index = to_tile_index(p, world_size.x);
			const auto our_ramp = m.ramp_layer[start_index];

			std::array<ramp_type, 4> ret [[indeterminate]];
			ret.fill(ramp_type::no_ramp);

			const auto cliff_layer = m.cliff_layer[start_index];
			for (auto i = rect_edges::begin; i < rect_edges::end; i = next(i))
			{
				const auto pos = adj_tiles[enum_type(i)] + p;
				if (within_world(pos, world_size))
				{
					const auto next_index = to_tile_index(pos, world_size.x);
					auto ramp_flags = true;
					if constexpr (CheckRampFlag)
					{
						const auto next_ramp = m.ramp_layer[next_index].test(enum_type(reverse_edges[enum_type(i)]));
						ramp_flags = our_ramp.test(enum_type(i)) && next_ramp;
					}

					const auto next_cliff_layer = m.cliff_layer[next_index];
					const auto ramp_type = to_ramp_type(cliff_layer, next_cliff_layer);

					if (ramp_flags)
						ret[enum_type(i)] = ramp_type;
				}
			}

			return ret;
		}

		static bool exposed_edge(const rect_edges e, const std::array<ramp_type, 4> r, ramp_type type) noexcept
		{
			using enum rect_edges;
			switch (e)
			{
			default:
				[[fallthrough]];
			case top:
				[[fallthrough]];
			case bottom:
				return r[enum_type(left)] == type || r[enum_type(right)] == type;
			case left:
			case right:
				return r[enum_type(top)] == type || r[enum_type(bottom)] == type;
			}
		}
	}

	adjacent_cliffs get_adjacent_cliffs(const tile_position p, const terrain_map& m)
	{
		const auto world_size = get_size(m);
		const auto start_index = to_tile_index(p, m);
		const auto our_layer = m.cliff_layer[start_index];
		const auto our_ramp = m.ramp_layer[start_index];
		const auto our_ramp_types = ramp::check_ramp_impl<true>(p, m);

		// check edges
		adjacent_cliffs out [[indeterminate]];
		for (auto i = rect_edges::begin; i < rect_edges::end; i = next(i))
		{
			const auto pos = ramp::adj_tiles[enum_type(i)] + p;
			auto cliff = std::int8_t{};
			if (within_world(pos, world_size))
			{
				const auto index = to_tile_index(pos, m);
				const auto cliff_layer = m.cliff_layer[index];
				cliff = integer_cast<std::int8_t>(cliff_layer - our_layer);

				const auto ramp = ramp::check_ramp_impl<true>(pos, m);
				if(ramp::exposed_edge(ramp::reverse_edges[enum_type(i)], ramp, ramp_type::uphill))
					cliff = our_layer + 1;
			}
			
			if (ramp::exposed_edge(i, our_ramp_types, ramp_type::downhill) && cliff >= 0)
				cliff = our_layer + 1;

			if(our_ramp.test(enum_type(i)))
				cliff = {};

			out.edges[enum_type(i)] = cliff;
		}

		// this function works well, but doesn't detect the gaps left by ramps
		// and accidently fills the centre of ramps with cliff texture
		//const auto tile_width = get_tile_width(m);
		//const auto index = to_tile_index(p, m);
		//const auto our_layer = m.cliff_layer[index];
		//const auto world_size = get_size(m);

		//const auto adjacent_tiles = std::array<tile_position, 4>{
		//	p - tile_position{ 0, 1 },
		//	p + tile_position{ 1, 0 },
		//	p + tile_position{ 0, 1 },
		//	p - tile_position{ 1, 0 }
		//};

		const auto to_cliff_layer = [&](const auto pos) {
			if (within_world(pos, world_size))
			{
				const auto i = to_tile_index(pos, m);
				const auto cliff_layer = m.cliff_layer[i];
				return integer_clamp_cast<std::int8_t>(cliff_layer - our_layer);
			}
			return std::int8_t{};
			};

		//adjacent_cliffs out [[indeterminate]];
		//std::ranges::transform(adjacent_tiles, std::begin(out.edges), to_cliff_layer);

		const auto adjacent_diag = std::array<tile_position, 4>{
			p + tile_position{ -1, -1 }, // top_left
			p + tile_position{ 1, -1 }, // top_right
			p + tile_position{ 1, 1 }, // bottom_right
			p + tile_position{ -1, 1 } // bottom_left
		};

		std::array<int8_t, 4> diag_cliffs [[indeterminate]];
		std::ranges::transform(adjacent_diag, std::begin(diag_cliffs), to_cliff_layer);

		out.corners = {
			out.edges[enum_type(rect_edges::top)] != 0 || out.edges[enum_type(rect_edges::left)] != 0 || diag_cliffs[enum_type(rect_corners::top_left)] != 0,
			out.edges[enum_type(rect_edges::top)] != 0 || out.edges[enum_type(rect_edges::right)] != 0 || diag_cliffs[enum_type(rect_corners::top_right)] != 0,
			out.edges[enum_type(rect_edges::bottom)] != 0 || out.edges[enum_type(rect_edges::right)] != 0 || diag_cliffs[enum_type(rect_corners::bottom_right)] != 0,
			out.edges[enum_type(rect_edges::bottom)] != 0 || out.edges[enum_type(rect_edges::left)] != 0 || diag_cliffs[enum_type(rect_corners::bottom_left)] != 0
		};

		return out;
	}


	static std::array<ramp_type, 4> get_adjacent_possible_ramps(const tile_position p, const terrain_map& m)
	{
		return ramp::check_ramp_impl(p, m);
	}

	// TODO: move this to a templated impl that can toggle the ramp flag check, to implement
	//		the function above without repeating all this code
	std::array<ramp_type, 4> get_adjacent_ramps(const tile_position p, const terrain_map& m)
	{
		return ramp::check_ramp_impl<true>(p, m);
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
			const auto ramp = m.ramp_layer[index].test(enum_type(ramp::reverse_edges[enum_type(edge)]));
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
	terrain_map::ramp_layer_t can_add_ramp(const tile_position p, const terrain_map& m)
	{
		const auto world_size = get_size(m);
		const auto starting_index = to_tile_index(p, world_size.x);
		const auto starting_layer = m.cliff_layer[starting_index];

		auto ret = terrain_map::ramp_layer_t{};
		constexpr auto end = ramp::adj_tiles.size();
		for (auto i = rect_edges::begin; i < rect_edges::end; i = next(i))
		{
			if (can_ramp(i, p, m) && !can_ramp(ramp::reverse_edges[enum_type(i)], p, m)) // we can only ramp if it's valid to ramp from both sides
				ret[enum_type(i)] = can_ramp_one_edge(ramp::reverse_edges[enum_type(i)], ramp::adj_tiles[enum_type(i)] + p, m);
		}

		return ret;		
	}

	void place_ramp(const tile_position p, terrain_map& m)
	{
		const auto ramp = can_add_ramp(p, m);
		const auto world_size = get_size(m);
		const auto starting_index = to_tile_index(p, world_size.x);
		for (auto i = rect_edges::begin; i < rect_edges::end; i = next(i))
		{
			if (ramp.test(enum_type(i)))
			{
				m.ramp_layer[starting_index].set(enum_type(i));
				const auto other_pos = ramp::adj_tiles[enum_type(i)] + p;
				const auto other_index = to_tile_index(other_pos, world_size.x);
				m.ramp_layer[other_index].set(enum_type(ramp::reverse_edges[enum_type(i)]));
			}
		}
		return;
	}

	void clear_ramp(const tile_position p, terrain_map& m)
	{
		const auto world_size = get_size(m);
		assert(within_world(p, world_size));
		const auto start_index = to_tile_index(p, world_size.x);
		auto& ramp = m.ramp_layer[start_index];
		
		// clear any adjacent ramp tiles that were connected to this one
		for (auto i = rect_edges::begin; i < rect_edges::end; i = next(i))
		{
			if (ramp.test(enum_type(i)))
			{
				const auto other_pos = ramp::adj_tiles[enum_type(i)] + p;
				const auto other_index = to_tile_index(other_pos, world_size.x);
				m.ramp_layer[other_index].reset(enum_type(i));
			}
		}

		ramp.reset();
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
			const auto height_info = get_height_for_triangles(tile_check, map, settings);
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
