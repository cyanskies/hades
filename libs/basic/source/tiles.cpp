#include "hades/tiles.hpp"

#include <numeric>
#include <tuple>

#include "hades/data.hpp"
#include "hades/resource_base.hpp"
#include "hades/table.hpp"

namespace hades
{
	using namespace std::string_view_literals;
	constexpr auto tilesets_name = "tilesets"sv;
	constexpr auto tile_settings_name = "tile-settings"sv;
	constexpr auto air_tileset_name = "air-tileset"sv;
	constexpr auto error_tileset_name = "error-tileset"sv;

	namespace id
	{
		unique_id tile_settings = unique_id::zero;
		unique_id error_tileset = unique_id::zero;
		unique_id empty_tileset = unique_id::zero;
	}

	void parse_tile_settings(unique_id mod, const data::parser_node&, data::data_manager&);
	void parse_tilesets(unique_id mod, const data::parser_node&, data::data_manager&);

	void register_tiles_resources(data::data_manager &d)
	{
		d.register_resource_type(tile_settings_name, parse_tile_settings);
		d.register_resource_type(tilesets_name, parse_tilesets);

		//create error tileset and add a default error tile
		id::error_tileset = hades::data::make_uid(error_tileset_name);
		auto error_tset = d.find_or_create<resources::tileset>(id::error_tileset, unique_id::zero);
		const resources::tile error_tile{};
		error_tset->tiles.emplace_back(error_tile);

		id::empty_tileset = hades::data::make_uid(air_tileset_name);
		auto empty_tset = d.find_or_create<resources::tileset>(id::empty_tileset, unique_id::zero);
		const resources::tile empty_tile{};
		empty_tset->tiles.emplace_back(empty_tile);

		//create default tile settings obj
		id::tile_settings = hades::data::make_uid(tile_settings_name);
		auto settings = d.find_or_create<resources::tile_settings>(id::tile_settings, unique_id::zero);
		settings->error_tileset = error_tset;
		settings->empty_tileset = empty_tset;
		settings->tile_size = 8;
	}
}

namespace hades::resources
{
	bool operator==(const tile &lhs, const tile &rhs)
	{
		return lhs.left == rhs.left &&
			lhs.texture == rhs.texture &&
			lhs.top == rhs.top;
	}

	bool operator!=(const tile &lhs, const tile &rhs)
	{
		return !(lhs == rhs);
	}

	bool operator<(const tile &lhs, const tile &rhs)
	{
		return std::tie(lhs.texture, lhs.left, lhs.top)
			< std::tie(rhs.texture, rhs.left, rhs.top);
	}

	void load_tileset(resource_type<tileset_t> &r, data::data_manager &d)
	{
		assert(dynamic_cast<tileset*>(&r));

		const auto &tset = static_cast<tileset&>(r);

		for (auto &t : tset.tiles)
		{
			if (t.texture)
			{
				const auto tex = reinterpret_cast<const resource_base*>(t.texture);
				data::get<texture>(tex->id);
			}
		}
	}

	void load_tile_settings(resource_type<tile_settings_t> &r, data::data_manager &d)
	{
		assert(dynamic_cast<tile_settings*>(&r));

		const auto &s = static_cast<tile_settings&>(r);

		if (s.empty_tileset)
			data::get<tileset>(s.empty_tileset->id);

		if (s.error_tileset)
			data::get<tileset>(s.error_tileset->id);
	}

	tileset::tileset() : resource_type<tileset_t>(load_tileset)
	{}

	tile_settings::tile_settings() : resource_type<tile_settings_t>(load_tile_settings)
	{}

	const tile_settings &get_tile_settings()
	{
		return *data::get<tile_settings>(id::tile_settings);
	}

	const tile &get_error_tile()
	{
		const auto &s = get_tile_settings();
		assert(s.error_tileset);
		const auto tset = s.error_tileset;
		const auto begin = std::begin(tset->tiles);
		const auto end = std::end(tset->tiles);
		return random_element(begin, end);
	}

	const tile &get_empty_tile()
	{
		const auto &s = get_tile_settings();
		assert(s.empty_tileset);
		const auto tset = s.empty_tileset;
		assert(!tset->tiles.empty());
		return tset->tiles.front();
	}
}

namespace hades
{
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
				const auto end_id = [iter2, end, starting_id, tileset] {
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
				std::move(std::begin(new_entries), std::end(new_entries),
					std::back_inserter(tile_swap));

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

	const resources::tileset *get_parent_tileset(const resources::tile &t)
	{
		const auto &s = resources::get_tile_settings();
		
		for (const auto *tileset : s.tilesets)
			for (const auto &tile : tileset->tiles)
				if (tile == t)
					return tileset;

		return nullptr;
	}

	const resources::tile &get_tile(const raw_map &r, tile_count_t t)
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

	const resources::tile &get_tile(const tile_map &m, tile_count_t t)
	{
		tile_count_t start{};

		for (const auto &tileset : m.tilesets)
		{
			assert(t > start);
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

	static tile_count_t flat_position(tile_position p, tile_count_t w)
	{
		if (p.x < 0 || p.y < 0)
			throw invalid_argument{ "cannot caculate the position of a tile in negative space" };

		//unsigned position
		const auto u_p = static_cast<vector_t<tile_count_t>>(p);
		return (u_p.y * w) + u_p.x;
	}

	const resources::tile &get_tile_at(const tile_map &t, tile_position p)
	{
		const auto index = flat_position(p, t.width);
		return get_tile(t, t.tiles[index]);
	}

	const tag_list &get_tags_at(const tile_map &t, tile_position p)
	{
		const auto &tile = get_tile_at(t, p);
		return tile.tags;
	}

	tile_map make_map(tile_position s, const resources::tile &t)
	{
		if (s.x < 0 || s.y < 0)
			throw invalid_argument{ "cannot make a tile map with a negative size" };

		const auto tileset = get_parent_tileset(t);
		if (!tileset)
			throw tileset_not_found{ "unable to find the tileset for tile passed into make_map" };

		auto m = tile_map{};
		m.tilesets.emplace_back(tileset);
		m.width = s.y;

		const auto index = get_tile_id(m, t);

		const auto length = static_cast<std::size_t>(s.x * s.y);
		m.tiles.reserve(length);
		std::fill_n(std::back_inserter(m.tiles), length, index);

		return m;
	}

	void resize_map_relative(tile_map &m, vector_int top_left, vector_int bottom_right, const resources::tile &t)
	{
		const auto current_height = m.tiles.size() / m.width;
		const auto current_width = m.width;

		const auto new_height = current_height - top_left.y + bottom_right.y;
		const auto new_width = current_width - top_left.x + bottom_right.x;

		resize_map(m, { new_width, new_height }, { -top_left.x, -top_left.y }, t);
	}

	void resize_map_relative(tile_map &t, vector_int top_left, vector_int bottom_right)
	{
		try
		{
			const auto empty_tile = resources::get_empty_tile();
			resize_map_relative(t, top_left, bottom_right, empty_tile);
		}
		catch (const data::resource_error &e)
		{
			throw tileset_not_found{ "unable to find the empty tileset" };
		}
	}

	void resize_map(tile_map &m, vector_int size, vector_int offset, const resources::tile &t)
	{
		const auto tile = make_tile_id(m, t);
		const auto new_map = always_table(vector_int{}, vector_int{ size.x, size.y }, tile);

		auto current_map = table<tile_count_t>{ offset,
			size, tile_count_t{} };

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

		std::copy(std::begin(new_tiles), std::end(new_tiles), std::begin(m.tiles));
	}

	void resize_map(tile_map &t, vector_int size, vector_int offset)
	{
		try
		{
			const auto empty_tile = resources::get_empty_tile();
			resize_map(t, size, offset, empty_tile);
		}
		catch (const data::resource_error &e)
		{
			throw tileset_not_found{ "unable to find the empty tileset" };
		}
	}

	std::vector<tile_position> make_position_square(tile_position position, tile_count_t size)
	{
		return make_position_rect(position, { size, size });
	}

	std::vector<tile_position> make_position_square_from_centre(tile_position middle, tile_count_t half_size)
	{
		return make_position_square({ middle.x - half_size, middle.y - half_size }, half_size * 2);
	}

	std::vector<tile_position> make_position_rect(tile_position position, tile_position size)
	{
		auto positions = std::vector<tile_position>{};
		positions.reserve(size.x * size.y);

		for (tile_count_t y = 0; y < size.y; ++y)
			for (tile_count_t x = 0; x < size.x; ++x)
				positions.emplace_back(position.x + x, position.y + y);

		return positions;
	}

	std::vector<tile_position> make_position_circle(tile_position middle, tile_count_t radius)
	{
		//TODO: 
		//? not sure how to implement this
		return std::vector<tile_position>();
	}
}
