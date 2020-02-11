#include "hades/tiles.hpp"

#include <array>
#include <numeric>

#include "hades/data.hpp"
#include "hades/parser.hpp"
#include "hades/resource_base.hpp"
#include "hades/table.hpp"

namespace hades
{
	namespace detail
	{
		static find_make_texture_f find_make_texture{};
	}

	//using namespace std::string_literals;
	using namespace std::string_view_literals;
	constexpr auto tilesets_name = "tilesets"sv;
	constexpr auto tile_settings_name = "tile-settings"sv;
	constexpr auto air_tileset_name = "air-tileset"sv;
	constexpr auto error_tileset_name = "error-tileset"sv;

	namespace id
	{
		static unique_id tile_settings = unique_id::zero;
		static unique_id error_tileset = unique_id::zero;
		static unique_id empty_tileset = unique_id::zero;
	}

	static void parse_tile_settings(unique_id mod, const data::parser_node&, data::data_manager&);
	static void parse_tilesets(unique_id mod, const data::parser_node&, data::data_manager&);

	void register_tiles_resources(data::data_manager &d)
	{
		register_tiles_resources(d, [](data::data_manager&, unique_id, unique_id)->resources::texture*
		{
			return nullptr;
		});
	}

	void register_tiles_resources(data::data_manager &d, detail::find_make_texture_f fm_texture)
	{
		d.register_resource_type(tile_settings_name, parse_tile_settings);
		d.register_resource_type(tilesets_name, parse_tilesets);

		const auto error_texture = unique_id{};
		auto texture = std::invoke(fm_texture, d, error_texture, unique_id::zero);

		//create error tileset and add a default error tile
		id::error_tileset = hades::data::make_uid(error_tileset_name);
		auto error_tset = d.find_or_create<resources::tileset>(id::error_tileset, unique_id::zero);
		const resources::tile error_tile{ texture, 0u, 0u, error_tset };
		error_tset->tiles.emplace_back(error_tile);

		id::empty_tileset = hades::data::make_uid(air_tileset_name);
		auto empty_tset = d.find_or_create<resources::tileset>(id::empty_tileset, unique_id::zero);
		if (std::empty(empty_tset->tiles))
		{
			const resources::tile empty_tile{ nullptr, 0u, 0u, empty_tset };
			empty_tset->tiles.emplace_back(empty_tile);
		}

		//create default tile settings obj
		id::tile_settings = hades::data::make_uid(tile_settings_name);
		auto settings = d.find_or_create<resources::tile_settings>(id::tile_settings, unique_id::zero);
		settings->error_tileset = error_tset;
		settings->empty_tileset = empty_tset;
		settings->tile_size = 8;

		detail::find_make_texture = fm_texture;
	}

	static void parse_tile_settings(unique_id mod, const data::parser_node &n, data::data_manager &d)
	{
		//tile-settings:
		//  tile-size: 32
		//	error-tileset

		const auto id = d.get_uid(tile_settings_name);
		auto s = d.find_or_create<resources::tile_settings>(id, mod);
		assert(s);

		s->tile_size = data::parse_tools::get_scalar(n, "tile-size"sv, s->tile_size);
	}

	static void add_tiles_to_tileset(std::vector<resources::tile> &tile_list,
		const resources::texture *texture, texture_size_t left,
		texture_size_t top, tile_count_t width,	tile_count_t count,
		resources::tile_size_t tile_size)
	{
		using resources::tile_size_t;
		texture_size_t x = 0, y = 0;
		tile_count_t current_count{};
		
		while (current_count < count)
		{
			const auto t_x = left + x * tile_size;
			const auto t_y = top + y * tile_size;
			tile_list.emplace_back(resources::tile{
				texture,
				integer_cast<tile_size_t>(t_x),
				integer_cast<tile_size_t>(t_y)
			});

			if (++x == width)
			{
				x = 0;
				++y;
			}

			++current_count;
		}
	}

	void resources::detail::parse_tiles(unique_id mod, resources::tileset &tileset, tile_size_t tile_size, const data::parser_node &n, data::data_manager &d)
	{
		using namespace data::parse_tools;
		tileset.tags = get_unique_sequence(n, "tags"sv, tileset.tags);

		const auto tile_groups_parent = n.get_child("tiles"sv);
		if (!tile_groups_parent)
			return;

		const auto tile_groups = tile_groups_parent->get_children();
		if (tile_groups.empty())
			return;

		for (const auto &tile_group : tile_groups)
		{
			const auto tex_id = get_unique(*tile_group, "texture"sv, unique_id::zero);

			if (tex_id == unique_id::zero)
			{
				const auto name = d.get_as_string(tileset.id);
				const auto m = "error parsing tileset: " + name + ", tilegroup is missing texture";
				LOGERROR(m);
				continue;
			}

			const resources::texture *tex =
				std::invoke(hades::detail::find_make_texture, d, tex_id, mod);

			const auto left = get_scalar(*tile_group, "left"sv, texture_size_t{});
			const auto top = get_scalar(*tile_group, "top"sv, texture_size_t{});

			const auto width = get_scalar(*tile_group, "tiles-per-row"sv, tile_count_t{});

			if (width == tile_count_t{})
			{
				const auto name = d.get_as_string(tileset.id);
				const auto m = "error parsing tileset: " + name + ", tiles-per-row is 0";
				LOGERROR(m);
				continue;
			}

			const auto tile_count = get_scalar(*tile_group, "tile-count"sv, tile_count_t{});

			if (tile_count == tile_count_t{})
			{
				const auto name = d.get_as_string(tileset.id);
				const auto m = "error parsing tileset: " + name + ", tile-count is 0";
				LOGERROR(m);
				continue;
			}

			add_tiles_to_tileset(tileset.tiles, tex, left, top, width,
				tile_count, tile_size);
		}
	}

	static void parse_tilesets(unique_id mod, const data::parser_node &n, data::data_manager &d)
	{
		//tilesets:
		//	sand: <// tileset name, these must be unique
		//		tags: <// a list of trait tags that get added to the tiles in this tileset; default: []
		//		tiles:
		//			- {
		//				texture: <// texture to draw the tiles from
		//				left: <// pixel start of tileset; default: 0
		//				top: <// pixel left of tileset; default: 0
		//				tiles_per_row: <// number of tiles per row
		//				tile_count: <// total amount of tiles in tileset; default: tiles_per_row
		//			}

		auto tile_settings = d.find_or_create<resources::tile_settings>(id::tile_settings, mod);
		assert(tile_settings);
		const auto tileset_list = n.get_children();

		const auto tile_size = tile_settings->tile_size;

		for (const auto &tileset_n : tileset_list)
		{
			const auto name = tileset_n->to_string();
			const auto id = d.get_uid(name);

			auto tileset = d.find_or_create<resources::tileset>(id, mod);

			resources::detail::parse_tiles(mod, *tileset, tile_size, *tileset_n, d);

			tile_settings->tilesets.emplace_back(tileset);
		}

		remove_duplicates(tile_settings->tilesets);
	}
}

namespace hades::resources
{
	std::string_view get_tilesets_name() noexcept
	{
		return tilesets_name;
	}

	std::string_view get_tile_settings_name() noexcept
	{
		return tile_settings_name;
	}

	std::string_view get_empty_tileset_name() noexcept
	{
		return air_tileset_name;
	}

	bool operator==(const tile &lhs, const tile &rhs)
	{
		return lhs.left == rhs.left &&
			lhs.texture == rhs.texture &&
			lhs.source == rhs.source &&
			lhs.top == rhs.top;
	}

	bool operator!=(const tile &lhs, const tile &rhs)
	{
		return !(lhs == rhs);
	}

	bool operator<(const tile &lhs, const tile &rhs)
	{
		return std::tie(lhs.texture, lhs.left, lhs.top, lhs.source)
			< std::tie(rhs.texture, rhs.left, rhs.top, rhs.source);
	}

	static void load_tileset(resource_type<tileset_t> &r, data::data_manager &d)
	{
		assert(dynamic_cast<tileset*>(&r));

		auto &tset = static_cast<tileset&>(r);

		for (auto &t : tset.tiles)
		{
			if (t.texture)
			{
				const auto res = reinterpret_cast<const resource_base*>(t.texture);
				auto texture = d.get_resource(res->id);
				if(!texture->loaded)
					texture->load(d);
			}
		}

		tset.loaded = true;
	}

	void detail::load_tile_settings(resource_type<tile_settings_t> &r, data::data_manager &d)
	{
		assert(dynamic_cast<tile_settings*>(&r));

		auto &s = static_cast<tile_settings&>(r);

		if (s.empty_tileset)
			d.get<tileset>(s.empty_tileset->id);

		if (s.error_tileset)
			d.get<tileset>(s.error_tileset->id);

		for (const auto t : s.tilesets)
			d.get<tileset>(t->id);

		s.loaded = true;
	}

	tileset::tileset() : resource_type{ load_tileset } {}

	tileset::tileset(loader_func f) : resource_type{ f } {}

	tile_settings::tile_settings() : resource_type{ detail::load_tile_settings }
	{}

	tile_settings::tile_settings(loader_func f) : resource_type{ f }
	{}

	const tile_settings *get_tile_settings()
	{
		return data::get<tile_settings>(id::tile_settings);
	}

	tile get_error_tile()
	{
		const auto s = get_tile_settings();
		assert(s->error_tileset);
		const auto tset = s->error_tileset;
		const auto begin = std::begin(tset->tiles);
		const auto end = std::end(tset->tiles);
		return random_element(begin, end);
	}

	tile get_empty_tile()
	{
		const auto s = get_tile_settings();
		assert(s->empty_tileset);
		const auto tset = s->empty_tileset;
		assert(!tset->tiles.empty());
		return tset->tiles.front();
	}

	unique_id get_tile_settings_id() noexcept
	{
		return id::tile_settings;
	}
}

namespace hades
{
	constexpr auto tilesets_str = "tilesets";
	constexpr auto map_str = "map";
	constexpr auto width_str = "width";

	void write_raw_map(const raw_map &m, data::writer &w)
	{
		//NOTE: w should already be pointing at tile_layer
		//tile_layer:
		//    tilesets:
		//        - [name, gid]
		//    map: [1,2,3,4...]
		//    width: 1

		//we should be pointing at a tile_layer

		w.start_sequence(tilesets_str);
		for (const auto [id, gid] : m.tilesets)
		{
			w.start_sequence();
			w.write(id);
			w.write(gid);
			w.end_sequence();
		}
		w.end_sequence();

		w.write(map_str, ""sv);
		w.start_sequence();
		for (const auto t : m.tiles)
			w.write(t);
		w.end_sequence();

		w.write(width_str, m.width);
	}

	raw_map read_raw_map(const data::parser_node &p)
	{
		//tile_layer:
		//    tilesets:
		//        - [name, gid]
		//    map: [1,2,3,4...]
		//    width: 1

		raw_map map{};

		//parser should be pointing at a tile_layer
		//tilesets
		const auto tilesets_node = p.get_child(tilesets_str);
		assert(tilesets_node);
		const auto tilesets = tilesets_node->get_children();
		for (const auto &tileset : tilesets)
		{
			assert(tileset);
			const auto tileset_data = tileset->get_children();
			assert(tileset_data.size() > 1);
			const auto name = tileset_data[0]->to_string();
			const auto count = tileset_data[1]->to_scalar<tile_count_t>();
			const auto id = data::get_uid(name);

			map.tilesets.emplace_back(tileset_info{id, count});
		}

		//map content
		const auto map_node = p.get_child(map_str);
		const auto tiles = map_node->to_sequence<tile_count_t>();
		
		map.tiles.insert(std::end(map.tiles), std::begin(tiles), std::end(tiles));

		//width
		const auto width = p.get_child(width_str);
		map.width = width->to_scalar<tile_count_t>();

		return map;
	}

	template<typename T>
	static tile_position get_size_impl(const T &t)
	{
		if (t.width == 0 ||
			t.tiles.empty())
			throw tile_error{ "malformed tile_map" };

		return tile_position{
			integer_cast<tile_position::value_type>(t.width),
			integer_cast<tile_position::value_type>(t.tiles.size() / t.width)
		};
	}

	int32 to_tiles(int32 pixels, resources::tile_size_t tile_size)
	{
		return pixels / tile_size;
	}

	tile_position to_tiles(vector_int pixels, resources::tile_size_t tile_size)
	{
		return pixels / integer_cast<vector_int::value_type>(tile_size);
	}

	int32 to_pixels(int32 tiles, resources::tile_size_t tile_size)
	{
		return tiles * tile_size;
	}

	vector_int to_pixels(tile_position tiles, resources::tile_size_t tile_size)
	{
		return tiles * integer_cast<tile_position::value_type>(tile_size);
	}

	tile_position get_size(const raw_map &t)
	{
		return get_size_impl(t);
	}

	tile_position get_size(const tile_map &t)
	{
		return get_size_impl(t);
	}

	tile_map to_tile_map(const raw_map &r)
	{
		//tile_maps must be editable
		//this means that any tile in the tilemap could be added to the map
		//so tile ids must be within the range of the full tileset size
		//rather that whatever range they are being stored in the raw map

		//we cant save a jagged map
		if (r.tiles.size() % r.width != 0)
			throw tile_error{ "raw_map is in an invalid state" };

		//we'll store a replacement table in tile swap 
		std::vector<tile_count_t> tile_swap;
		tile_count_t start{};
		const auto end = std::end(r.tilesets);
		for (auto iter = std::begin(r.tilesets); iter != end; ++iter)
		{
			//get the current starting id
			const auto[id, starting_id] = *iter;

			if (id == unique_id::zero)
				throw tileset_not_found{ "tileset id was unique_id::zero" };

			try
			{
				const auto tileset = data::get<resources::tileset>(id);

				//get the next starting id
				const auto iter2 = std::next(iter);
				const auto end_id = [iter2, end, starting_id = starting_id, tileset] {
					if (iter2 != end)
						return std::get<tile_count_t>(*iter2);
					else
						return starting_id + tileset->tiles.size();
				}();

				//the difference between this starting id and the next, is the number
				//of tiles that should be in this tileset
				const auto amount = end_id - starting_id;
				std::vector<tile_count_t> new_entries(std::size_t{ amount }, tile_count_t{});
				assert(new_entries.size() == amount); //vague initiliser list constructors :/

				//write the new tile ids into an array
				std::iota(std::begin(new_entries), std::end(new_entries), start);
				//move the entries into the swap array
				tile_swap.insert(std::end(tile_swap), std::begin(new_entries), std::end(new_entries));

				//array should now contain [old_id] = new_id
				start += tileset->tiles.size();
			}
			catch (const data::resource_error &e) // can be thrown from get<tileset>
			{
				using namespace std::string_literals;
				throw tileset_not_found{ "unable to access tilemap: "s + e.what() };
			}
		}

		tile_map t;
		t.width = r.width;
		//use the replacement table to swap out the tile ids
		t.tiles.reserve(r.tiles.size());
		std::transform(std::begin(r.tiles), std::end(r.tiles), std::back_inserter(t.tiles), [&tile_swap](auto &&t) {
			assert(t < tile_swap.size());
			return tile_swap[t];
		});

		//get the tileset ptrs
		t.tilesets.reserve(r.tilesets.size());
		std::transform(std::begin(r.tilesets), std::end(r.tilesets), std::back_inserter(t.tilesets), [](auto &&t) {
			const auto id = std::get<unique_id>(t);
			return data::get<resources::tileset>(id);
		});

		return t;
	}

	raw_map to_raw_map(const tile_map &t)
	{
		//we cant save a jagged map
		if (t.tiles.size() % t.width != 0)
			throw tile_error{ "tile_map is in an invalid state" };

		//the tiles id's and width will be unchanged
		raw_map m;
		m.tiles = t.tiles;
		m.width = t.width;

		//the starting id for each tileset is the size
		// of all preceeding tilesets
		decltype(t.tiles.size()) starting_id{};
		m.tilesets.reserve(t.tilesets.size());
		for (const auto &tileset : t.tilesets)
		{
			m.tilesets.emplace_back(tileset->id, starting_id);
			starting_id += tileset->tiles.size();
		}

		return m;
	}

	static bool is_in_tileset(const resources::tileset &tileset, const resources::tile &t)
	{
		for (const auto &tile : tileset.tiles)
			if (tile == t)
				return true;

		return false;
	}

	const resources::tileset *get_parent_tileset(const resources::tile &t)
	{
		const auto s = resources::get_tile_settings();
		
		for (const auto tileset : s->tilesets)
			if (is_in_tileset(*tileset, t))
				return tileset;

		//check if in empty
		if (is_in_tileset(*s->empty_tileset, t))
			return s->empty_tileset;

		//check error
		if (is_in_tileset(*s->error_tileset, t))
			return s->error_tileset;

		return nullptr;
	}

	resources::tile get_tile(const raw_map &r, tile_count_t t)
	{
		try
		{
			for (const auto[id, start] : r.tilesets)
			{
				const auto tileset = data::get<resources::tileset>(id);
				assert(t > start);
				if (t < start + tileset->tiles.size())
				{
					const auto index = t - start;
					return tileset->tiles[index];
				}
			}
		}
		catch (const data::resource_error &e)
		{
			using namespace std::string_literals;
			throw tileset_not_found{ "unable to find tileset in raw_map; "s + e.what() };
		}

		throw tile_not_found{ "tile is outside the bounds of the tilesets in this raw_map" };
	}

	resources::tile get_tile(const tile_map &m, tile_count_t t)
	{
		tile_count_t start{};

		for (const auto &tileset : m.tilesets)
		{
			if (t < start + tileset->tiles.size())
			{
				const auto index = t - start;
				return tileset->tiles[index];
			}

			start += tileset->tiles.size();
		}

		throw tile_not_found{ "tile is outside the bounds of the tilesets in this tile_map" };
	}

	tile_count_t get_tile_id(const raw_map &r, const resources::tile &t)
	{
		try
		{
			tile_count_t offset{};

			for (const auto &tset : r.tilesets)
			{
				const auto tileset = data::get<resources::tileset>(std::get<unique_id>(tset));
				const auto begin = std::begin(tileset->tiles);
				const auto end = std::end(tileset->tiles);
				for (auto iter = begin; iter != end; ++iter)
				{
					if (*iter == t)
						return offset + std::distance(begin, iter);
				}

				offset += std::size(tileset->tiles);
			}
		}
		catch (const data::resource_error &e)
		{
			using namespace std::string_literals;
			throw tileset_not_found{ "one of the tilesets in this raw map could not be found"s + e.what() };
		}

		throw tile_not_found{ "tile is outside the bounds of the tilesets in this raw_map" };
	}

	tile_count_t get_tile_id(const tile_map &m, const resources::tile &t)
	{
		tile_count_t offset{};

		for (const auto *tileset : m.tilesets)
		{
			const auto begin = std::begin(tileset->tiles);
			const auto end = std::end(tileset->tiles);
			for (auto iter = begin; iter != end; ++iter)
			{
				if (*iter == t)
					return offset + std::distance(begin, iter);		
			}

			offset += std::size(tileset->tiles);
		}

		throw tile_not_found{ "tile is outside the bounds of the tilesets in this tile_map" };
	}

	tile_count_t make_tile_id(tile_map &m, const resources::tile &t)
	{
		tile_count_t offset{};

		for (const auto *tileset : m.tilesets)
		{
			const auto begin = std::begin(tileset->tiles);
			const auto end = std::end(tileset->tiles);
			for (auto iter = begin; iter != end; ++iter)
			{
				if (*iter == t)
					return offset + std::distance(begin, iter);
			}

			offset += std::size(tileset->tiles);
		};

		const auto tileset = get_parent_tileset(t);

		if (!tileset)
			throw tileset_not_found{ "unable to find a tileset containing this tile" };

		m.tilesets.emplace_back(tileset);
		for (auto iter = std::begin(tileset->tiles); iter != std::end(tileset->tiles); ++iter)
		{
			if (*iter == t)
				return offset + std::distance(std::begin(tileset->tiles), iter);
		}

		throw tile_error{ "unable to add tile id for this tile_map" };
	}

	const tag_list &get_tags(const resources::tile &t)
	{
		//TODO: if source == null, is bad tag
		assert(t.source);
		return t.source->tags;
	}

	resources::tile get_tile_at(const tile_map &t, tile_position p)
	{
		const auto index = to_1d_index(p, t.width);
		assert(index < std::size(t.tiles));
		return get_tile(t, t.tiles[index]);
	}

	const tag_list &get_tags_at(const tile_map &t, tile_position p)
	{
		const auto &tile = get_tile_at(t, p);
		return get_tags(tile);
	}

	tile_map make_map(tile_position s, const resources::tile &t)
	{
		if (s.x < 0 || s.y < 0)
			throw invalid_argument{ "cannot make a tile map with a negative size" };

		auto m = tile_map{};
		m.width = s.x;

		const auto index = make_tile_id(m, t);

		const auto length = integer_cast<std::size_t>(s.x * s.y);
		m.tiles = std::vector<tile_count_t>(length, index);
		assert(m.tiles.size() == length);
		return m;
	}

	void resize_map_relative(tile_map &m, vector_int top_left, vector_int bottom_right, const resources::tile &t)
	{
		const auto current_height = m.tiles.size() / m.width;
		const auto current_width = m.width;

		const auto new_height = current_height - top_left.y + bottom_right.y;
		const auto new_width = current_width - top_left.x + bottom_right.x;

		const auto size = vector_int{
			integer_cast<int32>(new_width),
			integer_cast<int32>(new_height)
		};

		resize_map(m, size, { -top_left.x, -top_left.y }, t);
	}

	void resize_map_relative(tile_map &t, vector_int top_left, vector_int bottom_right)
	{
		const auto empty_tile = resources::get_empty_tile();
		resize_map_relative(t, top_left, bottom_right, empty_tile);
	}

	void resize_map(tile_map &m, vector_int size, vector_int offset, const resources::tile &t)
	{
		const auto tile = make_tile_id(m, t);
		const auto new_map = always_table{ vector_int{}, vector_int{ size.x, size.y }, tile };

		auto current_map = table<tile_count_t>{ offset,
			get_size(m), tile_count_t{} };

		auto &current_tiles = current_map.data();
		assert(current_tiles.size() == m.tiles.size());
		std::copy(std::begin(m.tiles), std::end(m.tiles), std::begin(current_tiles));

		const auto resized_map = combine_table(new_map, current_map, [](auto &&, auto &&rhs)
		{
			return rhs;
		});

		assert(resized_map.size() == size);
		
		m.width = size.x;
		const auto &new_tiles = resized_map.data();
		m.tiles = new_tiles;
	}

	void resize_map(tile_map &t, vector_int size, vector_int offset)
	{
		const auto empty_tile = resources::get_empty_tile();
		resize_map(t, size, offset, empty_tile);
	}

	void place_tile(tile_map &m, tile_position p, tile_count_t t)
	{
		if (p.x < 0 ||
			p.y < 0)
			return;

		//ignore placements outside of the map
		
		const auto index = to_1d_index(p, m.width);
		if (unsigned_cast(p.x) >= m.width ||
			index >= m.tiles.size())
			return;

		m.tiles[index] = t;
	}

	void place_tile(tile_map &m, tile_position p, const resources::tile &t)
	{
		const auto id = make_tile_id(m, t);
		place_tile(m, p, id);
	}

	void place_tile(tile_map &m, const std::vector<tile_position> &p, tile_count_t t)
	{
		for (const auto &pos : p)
			place_tile(m, pos, t);
	}

	void place_tile(tile_map &m, const std::vector<tile_position> &p, const resources::tile &t)
	{
		const auto id = make_tile_id(m, t);
		place_tile(m, p, id);
	}

	std::vector<tile_position> make_position_square(tile_position position, tile_count_t size)
	{
		const auto int_size = integer_cast<int32>(size);

		return make_position_rect(position, { int_size, int_size });
	}

	std::vector<tile_position> make_position_square_from_centre(tile_position middle, tile_count_t half_size)
	{
		const auto int_half_size = integer_cast<int32>(half_size);
		return make_position_square({ middle.x - int_half_size, middle.y - int_half_size }, int_half_size * 2);
	}

	std::vector<tile_position> make_position_rect(tile_position position, tile_position size)
	{
		if (size == tile_position{})
			return { position };

		auto positions = std::vector<tile_position>{};
		positions.reserve(size.x * size.y);

		for (int32 y = 0; y < size.y; ++y)
			for (int32 x = 0; x < size.x; ++x)
				positions.emplace_back(tile_position{ position.x + x, position.y + y });

		return positions;
	}

	std::vector<tile_position> make_position_circle(tile_position p, tile_count_t radius)
	{
		const auto rad = integer_cast<int32>(radius);

		const auto top = tile_position{ 0, -rad };
		const auto bottom = tile_position{ 0, rad };

		std::vector<tile_position> out;
		out.reserve(rad * 4);

		out.emplace_back(top + p);

		const auto r2 = rad * rad;
		for (auto y = top.y + 1; y < bottom.y; ++y)
		{
			//find x for every y between the top and bottom of the circle

			//x2 + y2 = r2
			//x2 = r2 - y2
			//x = sqrt(r2 - y2)
			const auto y2 = y * y;
			const auto a = r2 - y2;
			assert(a >= 0);
			const auto x_root = std::sqrt(static_cast<float>(a));
			const auto x_float = std::trunc(x_root);

			const auto x_int = std::abs(static_cast<tile_position::value_type>(x_float));
			const auto bounds = std::array{ -x_int, x_int };
			
			//push the entire line of the circle into out
			for (auto x = bounds[0]; x <= bounds[1]; x++)
				out.emplace_back(tile_position{ x + p.x, y + p.y });
		}

		out.emplace_back(bottom + p);

		return out;
	}
}
