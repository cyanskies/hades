#include "hades/terrain.hpp"

#include <map>

#include "hades/data.hpp"
#include "hades/parser.hpp"
#include "hades/random.hpp"
#include "hades/table.hpp"
#include "hades/tiles.hpp"
#include "hades/writer.hpp"

hades::unique_id background_terrain_id = {};

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
	constexpr auto terrains_str = "terrains"sv;

	void register_terrain_resources(data::data_manager &d, detail::make_texture_link_f func)
	{
		detail::make_texture_link = func;

		//create the terrain settings
		//and empty terrain first,
		//so that tiles can use them as tilesets without knowing
		//that they're really terrains
		id::terrain_settings = d.get_uid(resources::get_tile_settings_name());
		auto terrain_settings = d.find_or_create<resources::terrain_settings>(id::terrain_settings, {}, terrain_settings_str);

		const auto empty_tileset_id = d.get_uid(resources::get_empty_tileset_name());
		auto empty = d.find_or_create<resources::terrain>(empty_tileset_id, {}, terrains_str);
		
		//fill all the terrain tile lists with a default constructed tile
		apply_to_terrain(*empty, [empty](std::vector<resources::tile>&v) {
			v.emplace_back(resources::tile{ {}, 0u, 0u, empty });
		});
		
		terrain_settings->empty_terrain = d.make_resource_link<resources::terrain>(empty_tileset_id, id::terrain_settings);
		terrain_settings->empty_tileset = d.make_resource_link<resources::tileset>(empty_tileset_id, id::terrain_settings);

		const auto empty_tset_id = d.get_uid(resources::get_empty_terrainset_name());
		auto empty_tset = d.find_or_create<resources::terrainset>(empty_tset_id, {}, terrainsets_str);
		empty_tset->terrains.emplace_back(terrain_settings->empty_terrain);

		terrain_settings->empty_terrainset = d.make_resource_link<resources::terrainset>(empty_tset_id, id::terrain_settings);

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

		terrain_settings->background_terrain = d.make_resource_link<resources::terrain>(background_terrain_id, id::terrain_settings);


		//register tile resources
		register_tiles_resources(d, func);

		//replace the tileset and tile settings parsers
		using namespace std::string_view_literals;
		//register tile resources
		d.register_resource_type(resources::get_tile_settings_name(), resources::parse_terrain_settings);
		d.register_resource_type(terrain_settings_str, resources::parse_terrain_settings);
		d.register_resource_type(resources::get_tilesets_name(), resources::parse_terrain);
		d.register_resource_type("terrain"sv, resources::parse_terrain);
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

	static tile_corners make_empty_corners()
	{
		const auto empty = resources::get_empty_terrain();
		return std::array{ empty, empty, empty, empty };
	}

    static tile_corners get_terrain_at_tile(const std::vector<const resources::terrain*> &v, const terrain_index_t width, tile_position p)
	{
		auto out = make_empty_corners();

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
	static tile_map generate_layer(const std::vector<const resources::terrain*> &v, tile_index_t w, InputIt first, InputIt last)
	{
		//NOTE: w2 and h are 0 based, and one less than vertex width and height
		//this is good, it means they are equal to the tile width/height
		assert(!std::empty(v));
		const auto vertex_h = integer_cast<tile_index_t>(size(v)) / (w + 1);
		const auto h = vertex_h - 1;

		auto out = tile_map{};
		out.width = w;
		out.tilesets = std::vector<const resources::tileset*>{
			resources::get_empty_terrain(),
			first->get()
		};

		const auto size = w * h;

		for (auto i = tile_index_t{}; i < size; ++i)
		{
			const auto [x, y] = to_2d_index(i, w);
			assert(x < w);

			const auto corners = get_terrain_at_tile(v, w + 1, { integer_cast<int32>(x), integer_cast<int32>(y) });
			const auto type = get_transition_type(corners, first, last);
			const auto tile = resources::get_random_tile(**first, type);
			out.tiles.emplace_back(get_tile_id(out, tile));
		}

		return out;
	}

	static std::vector<tile_map> generate_terrain_layers(const resources::terrainset *t, const std::vector<const resources::terrain*> &v, tile_index_t width)
	{
		auto out = std::vector<tile_map>{};

		const auto end = std::crend(t->terrains);
		for (auto iter = std::crbegin(t->terrains); iter != end; ++iter)
			out.emplace_back(generate_layer(v, width, iter, end));

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

		const auto vertex_length = terrain_size.x * terrain_size.y;

		if (!std::empty(r.terrain_vertex) &&
			integer_cast<decltype(vertex_length)>(std::size(r.terrain_vertex)) != vertex_length)
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

	using namespace std::string_view_literals;
	constexpr auto terrainset_str = "terrainset"sv;
	constexpr auto terrain_vertex_str = "terrain-vertex"sv; // TODO: terrain-vertex
	constexpr auto terrain_layers_str = "terrain-layers"sv;// TODO: terrain-layers

	void write_raw_terrain_map(const raw_terrain_map & m, data::writer & w)
	{
		// we start in a terrain: node

		//terrainset
		//terrain vertex
		// tile layers

		using namespace std::string_view_literals;
		w.write(terrainset_str, m.terrainset);

		w.start_map(terrain_vertex_str); 
		w.start_sequence();
		for (const auto t : m.terrain_vertex)
			w.write(t);
		w.end_sequence();
		w.end_map();

		w.start_sequence(terrain_layers_str); 
		for (const auto &l : m.terrain_layers)
		{
			w.start_map();
			write_raw_map(l, w);
			w.end_map();
		}

		w.end_sequence();
	}

	std::tuple<unique_id, std::vector<terrain_id_t>, std::vector<raw_map>>
		read_raw_terrain_map(const data::parser_node &p)
	{
		//terrain:
		//	terrainset:
		//	terrain_vertex:
		//	terrain_layers:

		const auto terrainset = data::parse_tools::get_unique(p, terrainset_str, unique_zero);
		const auto terrain_vertex = data::parse_tools::get_sequence(p, terrain_vertex_str, std::vector<terrain_id_t>{});

		auto layers = std::vector<raw_map>{};

		const auto layer_node = p.get_child(terrain_layers_str);
		for (const auto &l : layer_node->get_children())
			layers.emplace_back(read_raw_map(*l));

		return { terrainset, terrain_vertex, layers };
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

		const auto settings = resources::get_terrain_settings();

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
		m.terrain_layers = generate_terrain_layers(m.terrainset, m.terrain_vertex, size.x);

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

				const auto raw_layer = std::find_if(begin(r.terrain_layers), end(r.terrain_layers), [t = (*terrain)->id](auto& l){
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

		assert(is_valid(to_raw_terrain_map(m)));

		return m;
	}

	raw_terrain_map to_raw_terrain_map(const terrain_map &t)
	{
		auto m = raw_terrain_map{};

		m.terrainset = t.terrainset->id;

		//build a replacement lookup table
		auto t_map = std::map<const resources::terrain*, terrain_id_t>{};

		const auto empty = resources::get_empty_terrain();
		//add the empty terrain with the index 0
		t_map.emplace(empty, terrain_id_t{});

		//add the rest of the terrains, offset the index by 1
		for (auto i = terrain_id_t{}; i < std::size(t.terrainset->terrains); ++i)
			t_map.emplace(t.terrainset->terrains[i].get(), i + 1u);

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
        map.terrain_vertex = std::vector<const resources::terrain*>(integer_cast<std::size_t>((size.x + 1) * (size.y + 1)), t);

		const auto empty_layer = make_map(size, resources::get_empty_tile());

		if (t != resources::get_empty_terrain())
		{
			//fill in the correct terrain layer
			const auto index = get_terrain_id(terrainset, t);

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
		return get_size(t.tile_layer) + tile_position{1, 1};
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

	const resources::terrain *get_corner(const tile_corners &t, rect_corners c) noexcept
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
        return m.terrain_vertex[integer_cast<std::size_t>(index)];
	}

	tag_list get_tags_at(const terrain_map &m, tile_position p)
	{
		const auto corners = get_terrain_at_tile(m, p);

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

	void resize_map_relative(terrain_map& m, vector_int top_left, vector_int bottom_right, const resources::terrain* t)
	{
        const auto current_height = integer_cast<tile_index_t>(m.tile_layer.tiles.size()) / m.tile_layer.width;
		const auto current_width = m.tile_layer.width;

        const auto new_height = current_height - top_left.y + bottom_right.y;
        const auto new_width = current_width - top_left.x + bottom_right.x;

		const auto size = vector_int{
            new_width,
            new_height
		};

		resize_map(m, size, { -top_left.x, -top_left.y }, t);
	}

	void resize_map_relative(terrain_map& m, vector_int top_left, vector_int bottom_right)
	{
		const auto terrain = resources::get_empty_terrain();
		resize_map_relative(m, top_left, bottom_right, terrain);
	}

	void resize_map(terrain_map& m, vector_int s, vector_int o, const resources::terrain* t)
	{
		const auto old_size = get_vertex_size(m);
		//resize tile layer
		resize_map(m.tile_layer, s, o);

		//update terrain vertex
		auto new_terrain = table_reduce_view{ vector_int{},
			s + vector_int{ 1, 1 }, t, [](auto&&, auto&& t) noexcept -> const resources::terrain* {
				return t;
		} };

		auto current_terrain = table_view{ o, old_size, m.terrain_vertex };

		new_terrain.add_table(current_terrain);
		
		auto vertex = std::vector<const resources::terrain*>{};
		const auto size = (s.x + 1) * (s.y + 1);
		vertex.reserve(size);

		for (auto i = vector_int::value_type{}; i < size; ++i)
			vertex.emplace_back(new_terrain[i]);

		m.terrain_vertex = std::move(vertex);

		//regenerate terrain layers
		m.terrain_layers = generate_terrain_layers(m.terrainset,
			m.terrain_vertex, integer_cast<terrain_index_t>(s.x));

		return;
	}

	void resize_map(terrain_map& m, vector_int size, vector_int offset)
	{
		const auto terrain = resources::get_empty_terrain();
		resize_map(m, size, offset, terrain);
	}

	std::vector<tile_position> get_adjacent_tiles(terrain_vertex_position p)
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

	std::vector<tile_position> get_adjacent_tiles(const std::vector<terrain_vertex_position> &v)
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
        m.terrain_vertex[integer_cast<std::size_t>(index)] = t;
	}

	static void update_tile_layers_internal(terrain_map &m, const std::vector<tile_position> &positions)
	{
		//remove tiles from the tile layer,
		//so that they dont obscure the new terrain
		place_tile(m.tile_layer, positions, resources::get_empty_tile());

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
				const auto corners = get_terrain_at_tile(m, p);

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
		const auto s = get_vertex_size(m);
		for (const auto& p : positions)
			if (within_map(s, p))
				place_terrain_internal(m, p, t);

		update_tile_layers(m, positions);
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

	static void load_terrain_settings(terrain_settings& s, data::data_manager &d)
	{
		detail::load_tile_settings(s, d);

		//load terrains
		if (s.empty_terrain)
			d.get<terrain>(s.empty_terrain->id);

		for (const auto& t : s.terrains)
			d.get<terrain>(t->id);

		for (const auto& t : s.terrainsets)
			d.get<terrainset>(t->id);

		s.loaded = true;
		return;
	}

	void terrain_settings::load(data::data_manager& d) 
	{
		load_terrain_settings(*this, d);
		return;
	}

	// if anyone asks for the 'none' type just give them the empty tile
	template<class U = std::vector<tile>, class W>
	U& get_transition(transition_tile_type type, W& t)
	{
		// is_const is only false for the functions inside this file
		// where we want to assert
		if constexpr (std::is_const_v<U>)
		{
			if (type == transition_tile_type::none)
				return get_empty_terrain()->tiles;
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

	static void add_tiles_to_terrain(terrain &terrain, const vector_int start_pos, resources::resource_link<resources::texture> tex,
		const std::vector<transition_tile_type> &tiles, const tile_index_t tiles_per_row, const tile_size_t tile_size)
	{
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

			auto &transition_vector = get_transition(t, terrain);

			const auto tile = resources::tile{ tex, integer_cast<tile_size_t>(tile_x), integer_cast<tile_size_t>(tile_y), &terrain };
			transition_vector.emplace_back(tile);
			terrain.tiles.emplace_back(tile);
		}
	}

	static std::vector<transition_tile_type> parse_layout(const std::vector<string> &s)
	{
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
			return { std::begin(war3_layout), std::end(war3_layout) };

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

		//short names, same values an order as the previous list
		constexpr auto short_names = std::array{
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

		//check against named layouts
		if (s.size() == 1u)
		{
			if (s[0] == "default"sv ||
				s[0] == "war3"sv)
				return { std::begin(war3_layout), std::end(war3_layout) };
		}

		std::vector<transition_tile_type> out;
		out.reserve(size(s));

		for (const auto &str : s)
		{
			assert(std::numeric_limits<uint8>::max() > std::size(transition_names));
			assert(std::size(transition_names) == std::size(short_names));
			for (auto i = uint8{}; i < std::size(transition_names); ++i)
			{
				if (transition_names[i] == str ||
					short_names[i] == str)
				{
					out.emplace_back(transition_tile_type{ i });
					break;
				}
			}

			//TODO: accept numbers?
			//		warn for missing str?
		}

		return out;
	}

	static void parse_terrain_group(terrain &t, const data::parser_node &p, data::data_manager &d, const tile_size_t tile_size)
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
		// TODO: handle texture_id == zero
		assert(texture_id);
		const auto texture = std::invoke(hades::detail::make_texture_link, d, texture_id, t.id);

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

			auto terrain = d.find_or_create<resources::terrain>(id, m, terrains_str);
			assert(terrain);
			//parse_tiles will fill in the tags and tiles
			resources::detail::parse_tiles(*terrain, tile_size, *terrain_node, d);

			const auto terrain_n = terrain_node->get_child("terrain"sv);
			if (terrain_n)
			{
				const auto terrain_group = terrain_n->get_children();
				for (const auto &group : terrain_group)
					parse_terrain_group(*terrain, *group, d, tile_size);
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
		//	name: [terrains, terrains, terrains]

		auto settings = d.find_or_create<terrain_settings>(resources::get_tile_settings_id(), mod, terrain_settings_str);

		const auto list = n.get_children();

		for (const auto &terrainset_n : list)
		{
			const auto name = terrainset_n->to_string();
			const auto id = d.get_uid(name);

			auto t = d.find_or_create<terrainset>(id, mod, terrainsets_str);

			auto unique_list = terrainset_n->merge_sequence(t->terrains, [id, &d](std::string_view s) {
				const auto i = d.get_uid(s);
				return d.make_resource_link<terrain>(i, id);
			});

			std::swap(unique_list, t->terrains);

			settings->terrainsets.emplace_back(d.make_resource_link<terrainset>(id, resources::get_tile_settings_id()));
		}

		remove_duplicates(settings->terrainsets);
	}

	static void parse_terrain_settings(unique_id mod, const data::parser_node& n, data::data_manager& d)
	{
		//tile-settings:
		//  tile-size: 32
		//	error-tileset

        //const auto id = d.get_uid(resources::get_tile_settings_name());
		auto s = d.find_or_create<resources::terrain_settings>(id::terrain_settings, mod, terrain_settings_str);
		assert(s);

		s->tile_size = data::parse_tools::get_scalar(n, "tile-size"sv, s->tile_size);
	}

	const terrain_settings *get_terrain_settings()
	{
		auto t = data::get<terrain_settings>(id::terrain_settings);
		assert(t);
		return t;
	}

	std::string_view get_empty_terrainset_name() noexcept
	{
		using namespace::std::string_view_literals;
		return "air-terrainset"sv;
	}

	const terrain *get_empty_terrain()
	{
		const auto settings = get_terrain_settings();
		assert(settings->empty_terrain);
		return settings->empty_terrain.get();
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
		assert(settings->empty_terrainset);
		return settings->empty_terrainset.get();
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
		const auto& vec = get_transitions(t, type);
		assert(!empty(vec));
		return *random_element(std::begin(vec), std::end(vec));
	}
}
