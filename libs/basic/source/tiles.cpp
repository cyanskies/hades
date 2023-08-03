#include "hades/tiles.hpp"

#include <array>
#include <numeric>

#include "hades/data.hpp"
#include "hades/deflate.hpp"
#include "hades/parser.hpp"
#include "hades/random.hpp"
#include "hades/resource_base.hpp"
#include "hades/table.hpp"
#include "hades/writer.hpp"

namespace hades
{
	namespace detail
	{
		static make_texture_link_f make_texture_link{};
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
		register_tiles_resources(d, 
			[](auto&, auto, auto)->resources::resource_link<resources::texture> {
				return {};
		});
	}

	void register_tiles_resources(data::data_manager &d, detail::make_texture_link_f get_texture)
	{
		d.register_resource_type(tile_settings_name, parse_tile_settings);
		d.register_resource_type(tilesets_name, parse_tilesets);

		id::error_tileset = hades::data::make_uid(error_tileset_name);

		const auto error_texture = d.get_uid("tile-error-texture"sv);
		const auto texture = std::invoke(get_texture, d, error_texture, id::error_tileset);

		//create error tileset and add a default error tile
		auto error_tset = d.find_or_create<resources::tileset>(id::error_tileset, {}, tilesets_name);
		const resources::tile error_tile{ texture, 0u, 0u, error_tset };
		error_tset->tiles = { error_tile };

		id::empty_tileset = hades::data::make_uid(air_tileset_name);
		auto empty_tset = d.find_or_create<resources::tileset>(id::empty_tileset, {}, tilesets_name);
		const resources::tile empty_tile{ nullptr, 0u, 0u, empty_tset };
		empty_tset->tiles = { empty_tile };

		//create default tile settings obj
		id::tile_settings = hades::data::make_uid(tile_settings_name);
		auto settings = d.find_or_create<resources::tile_settings>(id::tile_settings, {}, tile_settings_name);
		settings->error_tileset = d.make_resource_link<resources::tileset>(id::error_tileset, id::tile_settings);
		settings->empty_tileset = d.make_resource_link<resources::tileset>(id::empty_tileset, id::tile_settings);
		settings->tile_size = 8;

		detail::make_texture_link = get_texture;
	}

	static void parse_tile_settings(unique_id mod, const data::parser_node &n, data::data_manager &d)
	{
		//tile-settings:
		//  tile-size: 32

		// TODO: why dont we parse the error and empty tileset?
		//		need to update the serialise function if this changes

		const auto id = d.get_uid(tile_settings_name);
		auto s = d.find_or_create<resources::tile_settings>(id, mod, tile_settings_name);
		assert(s);

		s->tile_size = data::parse_tools::get_scalar(n, "tile-size"sv, s->tile_size);
	}

	static void add_tiles_to_tileset(std::vector<resources::tile> &tile_list,
		resources::resource_link<resources::texture> texture, texture_size_t left,
		texture_size_t top, tile_index_t width,	tile_index_t count,
		resources::tile_size_t tile_size)
	{
		using resources::tile_size_t;
		texture_size_t x = 0, y = 0;
		tile_index_t current_count{};
		
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

	void resources::detail::parse_tiles(resources::tileset &tileset, tile_size_t tile_size, const data::parser_node &n, data::data_manager &d)
	{
		using namespace data::parse_tools;
		
		tileset.tags = merge_unique_sequence(n, "tags"sv, tileset.tags);

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
				const auto& name = d.get_as_string(tileset.id);
				const auto m = "error parsing tileset: " + name + ", tilegroup is missing texture";
				LOGERROR(m);
				continue;
			}

			const auto tex = std::invoke(hades::detail::make_texture_link, d, tex_id, tileset.id);

			const auto left = get_scalar(*tile_group, "left"sv, texture_size_t{});
			const auto top = get_scalar(*tile_group, "top"sv, texture_size_t{});

			const auto width = get_scalar(*tile_group, "tiles-per-row"sv, tile_index_t{});

			if (width == tile_index_t{})
			{
				const auto& name = d.get_as_string(tileset.id);
				const auto m = "error parsing tileset: " + name + ", tiles-per-row is 0";
				LOGERROR(m);
				continue;
			}

			const auto tile_count = get_scalar(*tile_group, "tile-count"sv, tile_index_t{});

			if (tile_count == tile_index_t{})
			{
				const auto& name = d.get_as_string(tileset.id);
				const auto m = "error parsing tileset: " + name + ", tile-count is 0";
				LOGERROR(m);
				continue;
			}

			add_tiles_to_tileset(tileset.tiles, tex, left, top, width,
				tile_count, tile_size);

			tileset.tile_source_groups.emplace_back(tex_id, left, top, width, tile_count);
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

		auto tile_settings = d.find_or_create<resources::tile_settings>(id::tile_settings, mod, tile_settings_name);
		assert(tile_settings);
		const auto tileset_list = n.get_children();

		const auto tile_size = tile_settings->tile_size;

		for (const auto &tileset_n : tileset_list)
		{
			const auto name = tileset_n->to_string();
			const auto id = d.get_uid(name);

			auto tileset = d.find_or_create<resources::tileset>(id, mod, tilesets_name);

			resources::detail::parse_tiles(*tileset, tile_size, *tileset_n, d);

			tile_settings->tilesets.emplace_back(d.make_resource_link<resources::tileset>(id, id::tile_settings));
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

	unique_id get_empty_tileset_id() noexcept
	{
		return id::empty_tileset;
	}

	bool operator==(const tile &lhs, const tile &rhs) noexcept
	{
		return lhs.left == rhs.left &&
			lhs.tex == rhs.tex &&
			lhs.source == rhs.source &&
			lhs.top == rhs.top;
	}

	bool operator!=(const tile &lhs, const tile &rhs) noexcept
	{
		return !(lhs == rhs);
	}

	bool operator<(const tile &lhs, const tile &rhs) noexcept
	{
		return std::tie(lhs.tex, lhs.left, lhs.top, lhs.source)
			< std::tie(rhs.tex, rhs.left, rhs.top, rhs.source);
	}

	static void load_tileset(tileset& tset, data::data_manager &d)
	{
		for (auto &t : tset.tiles)
		{
			if (t.tex)
			{
				auto texture = d.get_resource(t.tex.id());
				if(!texture->loaded)
					texture->load(d);
			}
		}

		tset.loaded = true;
		return;
	}

	void detail::load_tile_settings(tile_settings &s, data::data_manager &d)
	{
		if (s.empty_tileset)
			d.get<tileset>(s.empty_tileset->id);

		if (s.error_tileset)
			d.get<tileset>(s.error_tileset->id);

		for (const auto t : s.tilesets)
			d.get<tileset>(t->id);

		s.loaded = true;
		return;
	}

	void tileset::load(data::data_manager& d)
	{
		load_tileset(*this, d);
		return;
	}

	void tileset::serialise(const data::data_manager& d, data::writer& w) const
	{
		w.start_map(d.get_as_string(id));
		serialise_impl(d, w);
		w.end_map();
		return;
	}

	void tileset::serialise_impl(const data::data_manager& d, data::writer& w) const
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

		if (empty(tags) ||
			empty(tiles))
			return;

		const auto prev = d.try_get_previous(this);

		// tags
		if (empty(tags) && prev.result && !empty(prev.result->tags))
		{
			w.write("tags"sv);
			w.start_sequence();
			w.write("="sv);
			w.end_sequence();
		}
		else if (!empty(tags))
		{
			const auto string_conv = [&d](auto u_id) {
				return d.get_as_string(u_id);
			};

			assert(std::ranges::is_sorted(tags));
			w.write("tags"sv);
			w.mergable_sequence({}, tags, prev.result ? prev.result->tags : tag_list{}, string_conv);
		}

		auto beg = begin(tile_source_groups);

		if (prev.result)
			advance(beg, size(prev.result->tile_source_groups));

		if (beg != end(tile_source_groups))
		{
			w.start_sequence("tiles"sv);
			std::for_each(beg, end(tile_source_groups), [&](auto g) {
				w.start_map();
				w.write("texture"sv, g.texture);
				if (g.top != texture_size_t{})
					w.write("top"sv, g.top);
				if (g.left != texture_size_t{})
					w.write("left"sv, g.left);
				w.write("tiles-per-row"sv, g.tiles_per_row);
				w.write("tile-count"sv, g.tile_count);
				w.end_map();
				});
			w.end_sequence();
		}

		return;
	}

	void tile_settings::load(data::data_manager& d)
	{
		detail::load_tile_settings(*this, d);
		return;
	}

	void tile_settings::serialise(const data::data_manager& d, data::writer& w) const
	{
		//tile-settings:
		//  tile-size: 32

		w.write("tile-size"sv, tile_size);
		return;
	}

	const tile_settings *get_tile_settings()
	{
		return data::get<tile_settings>(id::tile_settings);
	}

	tile_size_t get_tile_size()
	{
		const auto settings = get_tile_settings();
		assert(settings);
		return settings->tile_size;
	}

	const tile& get_error_tile()
	{
		const auto s = get_tile_settings();
		assert(s->error_tileset);
		const auto tset = s->error_tileset;
		const auto begin = std::begin(tset->tiles);
		const auto end = std::end(tset->tiles);
		assert(begin != end);
		return *random_element(begin, end);
	}

	const tile& get_empty_tile()
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
		//    map: [1,2,3,4...] OR 1
		//    width: 1

		//we should be pointing at a tile_layer

		w.start_sequence(tilesets_str);
        for (const auto& [id, gid] : m.tilesets)
		{
			w.start_sequence();
			w.write(id);
			w.write(gid);
			w.end_sequence();
		}
		w.end_sequence();

		const auto compressed = zip::deflate(m.tiles);
		w.write(map_str, base64_encode(compressed));

		/*w.write(map_str);
		w.start_sequence();
		for (const auto t : m.tiles)
			w.write(t);
		w.end_sequence();*/

		w.write(width_str, m.width);
		return;
	}

	raw_map read_raw_map(const data::parser_node &p, const std::size_t size)
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
			const auto count = tileset_data[1]->to_scalar<tile_index_t>();
			const auto id = data::get_uid(name);

			map.tilesets.emplace_back(tileset_info{id, count});
		}

		//map content
		if (auto map_node = p.get_child(map_str);
			map_node)
		{
			if(map_node->is_sequence())
				map.tiles = map_node->to_sequence<tile_id_t>();
			else
			{
				const auto map_encoded = map_node->to_string();
				const auto bytes = base64_decode<std::byte>(map_encoded);
				map.tiles = zip::inflate<tile_id_t>(bytes, size * sizeof(tile_id_t));
			}
		}

		//width
		const auto width = p.get_child(width_str);
		map.width = width->to_scalar<tile_index_t>();

		return map;
	}

	tile_position from_tile_index(tile_index_t i, tile_index_t w) noexcept
	{
		return to_2d_index<tile_position>(i, integer_cast<tile_index_t>(w));
	}

	tile_index_t to_tile_index(tile_position p, tile_index_t w) noexcept
	{
		return integer_cast<tile_index_t>(to_1d_index(p, w));
	}

	tile_position from_tile_index(tile_index_t i, const tile_map& t) noexcept
	{
		return from_tile_index(i, t.width);
	}

	tile_index_t to_tile_index(tile_position p, const tile_map& t) noexcept
	{
		return to_tile_index(p, t.width);
	}

	template<typename T>
	static tile_position get_size_impl(const T &t)
	{
		if (t.width == 0 ||
			t.tiles.empty())
			throw tile_error{ "malformed tile_map" };

		return tile_position{
			integer_cast<tile_position::value_type>(t.width),
            integer_cast<tile_position::value_type>(integer_cast<tile_index_t>(t.tiles.size()) / t.width)
		};
	}

	int32 to_tiles(int32 pixels, resources::tile_size_t tile_size) noexcept
	{
		assert(tile_size != 0);
		return pixels / tile_size;
	}

	tile_position to_tiles(vector_int pixels, resources::tile_size_t tile_size)
	{
		assert(tile_size != 0);
		return pixels / integer_cast<vector_int::value_type>(tile_size);
	}

	tile_position to_tiles(vector_float real_pixels, resources::tile_size_t tile_size)
	{
		return to_tiles(static_cast<vector_int>(real_pixels), tile_size);
	}

	int32 to_pixels(int32 tiles, resources::tile_size_t tile_size) noexcept
	{
		return tiles * tile_size;
	}

	vector_int to_pixels(tile_position tiles, resources::tile_size_t tile_size) noexcept
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
        if (r.tiles.size() % integer_cast<std::size_t>(r.width) != 0)
			throw tile_error{ "raw_map is in an invalid state" };

		//we'll store a replacement table in tile swap 
		std::vector<tile_id_t> tile_swap;
		auto start = std::size_t{};//tile_index_t start{};
		const auto end = std::end(r.tilesets);
		for (auto iter = std::begin(r.tilesets); iter != end; ++iter)
		{
			//get the current starting id
			const auto& [id, starting_id] = *iter;

			if (id == unique_id::zero)
				throw tileset_not_found{ "tileset id was unique_id::zero" };

			try
			{
				const auto tileset = data::get<resources::tileset>(id);

				//get the next starting id
				const auto iter2 = std::next(iter);
				const auto end_id = [iter2, end, starting_id = starting_id, tileset]() {
					if (iter2 != end)
						return std::get<tile_id_t>(*iter2);
					else
						return starting_id + tileset->tiles.size();
				}();

				//the difference between this starting id and the next, is the number
				//of tiles that should be in this tileset
				const std::size_t amount = end_id - starting_id;
				std::vector<tile_id_t> new_entries(amount, tile_id_t{});
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

		auto t = tile_map{};
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
        if (t.tiles.size() % integer_cast<std::size_t>(t.width) != 0)
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

	static bool is_in_tileset(const resources::tileset &tileset, const resources::tile &t) noexcept
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
				return tileset.get();

		//check if in empty
		if (is_in_tileset(*s->empty_tileset, t))
			return s->empty_tileset.get();

		//check error
		if (is_in_tileset(*s->error_tileset, t))
			return s->error_tileset.get();

		return nullptr;
	}

	resources::tile get_tile(const raw_map &r, tile_id_t t)
	{
		try
		{
            for (const auto& [id, start] : r.tilesets)
			{
				const auto tileset = data::get<resources::tileset>(id);
				assert(t > start);
				if (t < start + tileset->tiles.size())
				{
					const auto index = t - start;
					return tileset->tiles[integer_cast<std::size_t>(index)];
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

	resources::tile get_tile(const tile_map &m, tile_id_t t)
	{
		auto start = std::size_t{};

		for (const auto &tileset : m.tilesets)
		{
			if (t < start + tileset->tiles.size())
			{
				const auto index = t - start;
				return tileset->tiles[integer_cast<std::size_t>(index)];
			}

			start += tileset->tiles.size();
		}

		throw tile_not_found{ "tile is outside the bounds of the tilesets in this tile_map" };
	}

	tile_id_t get_tile_id(const raw_map &r, const resources::tile &t)
	{
		try
		{
			std::size_t offset{};

			for (const auto &tset : r.tilesets)
			{
				const auto tileset = data::get<resources::tileset>(std::get<unique_id>(tset));
				const auto begin = std::begin(tileset->tiles);
				const auto end = std::end(tileset->tiles);
				// 'iter' is an intended copy
				for (auto iter = begin; iter != end; ++iter)
				{
					if (*iter == t)
                        return offset + integer_cast<std::size_t>(std::distance(begin, iter));
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

	tile_id_t get_tile_id(const tile_map &m, const resources::tile &t)
	{
		std::size_t offset{};

		for (const auto *tileset : m.tilesets)
		{
			const auto begin = std::begin(tileset->tiles);
			const auto end = std::end(tileset->tiles);
			// 'iter' is an intended copy
			for (auto iter = begin; iter != end; ++iter)
			{
				if (*iter == t)
                    return offset + integer_cast<std::size_t>(std::distance(begin, iter));
			}

			offset += std::size(tileset->tiles);
		}

		throw tile_not_found{ "tile is outside the bounds of the tilesets in this tile_map" };
	}

	tile_id_t make_tile_id(tile_map &m, const resources::tile &t)
	{
		std::size_t offset{};

		for (const auto *tileset : m.tilesets)
		{
			const auto begin = std::begin(tileset->tiles);
			const auto end = std::end(tileset->tiles);
			// 'iter' is an intended copy
			for (auto iter = begin; iter != end; ++iter)
			{
				if (*iter == t)
                    return offset + integer_cast<std::size_t>(std::distance(begin, iter));
			}

			offset += std::size(tileset->tiles);
		};

		const auto tileset = get_parent_tileset(t);

		if (!tileset)
			throw tileset_not_found{ "unable to find a tileset containing this tile" };

		m.tilesets.emplace_back(tileset);
		const auto tileset_end = std::end(tileset->tiles);
		for (auto iter = std::begin(tileset->tiles); iter != tileset_end; ++iter)
		{
			if (*iter == t)
                return offset + integer_cast<std::size_t>(std::distance(std::begin(tileset->tiles), iter));
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
		const auto index = integer_cast<std::size_t>(to_tile_index(p, t));
		assert(index < size(t.tiles));
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
		m.tiles = std::vector<tile_id_t>(length, index);
		assert(m.tiles.size() == length);
		return m;
	}

	void resize_map_relative(tile_map &m, vector_int top_left, vector_int bottom_right, const resources::tile &t)
	{
        const auto current_height = integer_cast<int32>(m.tiles.size()) / m.width;
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
		const auto& empty_tile = resources::get_empty_tile();
		resize_map_relative(t, top_left, bottom_right, empty_tile);
	}

	void resize_map(tile_map &m, const vector_int size, const vector_int offset, const resources::tile &t)
	{
		const auto tile = make_tile_id(m, t);

		auto new_tilemap = table_reduce_view{ vector_int{},
			size, tile, [](auto&&, auto&& tile_id) noexcept -> tile_id_t {
				return tile_id;
		} };

		const auto old_size = get_size(m);
		auto current_map = table_view{ offset, old_size, m.tiles };

		new_tilemap.add_table(current_map);

		auto tiles = std::vector<tile_id_t>{};
		const auto arr_size = size.x * size.y;
		tiles.reserve(arr_size);

		for (auto i = vector_int::value_type{}; i < arr_size; ++i)
			tiles.emplace_back(new_tilemap[i]);

		m.tiles = std::move(tiles);
		m.width = size.x;

		return;
	}

	void resize_map(tile_map &t, vector_int size, vector_int offset)
	{
		const auto& empty_tile = resources::get_empty_tile();
		resize_map(t, size, offset, empty_tile);
	}

	void place_tile(tile_map &m, tile_position p, tile_id_t t)
	{
		if (p.x < 0 ||
			p.y < 0)
			return;

		//TODO: check that tile_id is within the maps id range(valid tile)

		//ignore placements outside of the map
        const auto index = integer_cast<std::size_t>(to_1d_index(p, m.width));
        if (p.x >= m.width || index >= m.tiles.size())
			return;

		m.tiles[index] = t;
	}

	void place_tile(tile_map &m, tile_position p, const resources::tile &t)
	{
		const auto id = make_tile_id(m, t);
		place_tile(m, p, id);
	}

	void place_tile(tile_map &m, const std::vector<tile_position> &p, tile_id_t t)
	{
		for (const auto &pos : p)
			place_tile(m, pos, t);
	}

	void place_tile(tile_map &m, const std::vector<tile_position> &p, const resources::tile &t)
	{
		const auto id = make_tile_id(m, t);
		place_tile(m, p, id);
	}

	std::vector<tile_position> make_position_square(tile_position position, tile_index_t size)
	{
		const auto int_size = integer_cast<int32>(size);

		return make_position_rect(position, { int_size, int_size });
	}

	std::vector<tile_position> make_position_square_from_centre(tile_position middle, tile_index_t half_size)
	{
		const auto int_half_size = integer_cast<int32>(half_size);
		return make_position_square({ middle.x - int_half_size, middle.y - int_half_size }, int_half_size * 2);
	}

	std::vector<tile_position> make_position_rect(tile_position position, tile_position size)
	{
		if (size == tile_position{})
			return { position };

		auto positions = std::vector<tile_position>{};
        positions.reserve(integer_cast<std::size_t>(size.x * size.y));

		for (int32 y = 0; y < size.y; ++y)
			for (int32 x = 0; x < size.x; ++x)
				positions.emplace_back(tile_position{ position.x + x, position.y + y });

		return positions;
	}

	std::vector<tile_position> make_position_circle(tile_position p, tile_index_t radius)
	{
		const auto rad = integer_cast<tile_position::value_type>(radius);

		const auto top = tile_position{ 0, -rad };
		const auto bottom = tile_position{ 0, rad };

		std::vector<tile_position> out;
        out.reserve(integer_cast<std::size_t>(rad * 4));

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

	std::array<tile_position, 9> make_position_9patch(tile_position m) noexcept
	{
		using p = tile_position;
		return std::array{
			m + p{-1, -1},	m + p{0, -1},	m + p{1, -1},
			m + p{-1, 0},	m,				m + p{1, 0}, 
			m + p{-1, 1},	m + p{0, 1},	m + p{1, 1}
		};
	}
}
