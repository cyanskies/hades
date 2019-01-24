#include "hades/tiles.hpp"

#include <numeric>
#include <tuple>

#include "hades/data.hpp"
#include "hades/resource_base.hpp"

namespace hades
{
	using namespace std::string_view_literals;
	constexpr auto tilesets_name = "tilesets"sv;
	constexpr auto tile_settings_name = "tile-settings"sv;

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
		id::error_tileset = unique_id{};
		auto error_tset = d.find_or_create<resources::tileset>(id::error_tileset, unique_id::zero);
		const resources::tile error_tile{};
		error_tset->tiles.emplace_back(error_tile);

		id::empty_tileset = unique_id{};
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
		const auto begin = std::begin(tset->tiles);
		const auto end = std::end(tset->tiles);
		return random_element(begin, end);
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
			for (const auto &tset : r.tilesets)
			{
				const auto tileset = data::get<resources::tileset>(std::get<unique_id>(tset));
				const auto begin = std::begin(tileset->tiles);
				const auto end = std::end(tileset->tiles);
				for (auto iter = begin; iter != end; ++iter)
				{
					if (*iter == t)
						return std::distance(begin, iter);
				}
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
		for (const auto *tileset : m.tilesets)
		{
			const auto begin = std::begin(tileset->tiles);
			const auto end = std::end(tileset->tiles);
			for (auto iter = begin; iter != end; ++iter)
			{
				if (*iter == t)
					return std::distance(begin, iter);		
			}
		}

		throw tile_not_found{ "tile is outside the bounds of the tilesets in this tile_map" };
	}
}
