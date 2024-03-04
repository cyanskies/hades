#ifndef HADES_TILES_HPP
#define HADES_TILES_HPP

#include <array>

#include "hades/data.hpp"
#include "hades/game_types.hpp"

// system for working with logical tiles
// editing a tilemap, and getting tag_lists from the map

// forward decs
namespace hades::data
{
	class writer;
	class parser_node;
}

namespace hades::resources
{
	//fwd declaration
	// tiles cannot be drawn unless linked against hades-core
	// which defines textures
	struct texture;
}

namespace hades::detail
{
	using make_texture_link_f = resources::resource_link<resources::texture> (*)(data::data_manager&, unique_id, unique_id);
}

namespace hades
{
	//NOTE:this doesn't register the texture resource
	// this is to allow logical gameplay tilemap usage
	// without bringing in graphics resources and linkage
	void register_tiles_resources(data::data_manager&);
	//as above, but loads textures from the requested function
	void register_tiles_resources(data::data_manager&, detail::make_texture_link_f);

	//x/y coords of a tile, negative tile positions aren't supported
	using tile_position = vector2_int;
	// 1d tile positions
	using tile_index_t = tile_position::value_type;
}

//resources for loading tiles and tilesets
namespace hades::resources
{
	std::string_view get_tilesets_name() noexcept;
	std::string_view get_tile_settings_name() noexcept;
	std::string_view get_empty_tileset_name() noexcept;
	unique_id get_empty_tileset_id() noexcept;
	
	//maximum tile size is capped by texture size
	using tile_size_t = texture_size_t;

	struct tileset;
	struct tile
	{
		//texture and [left, top] are used for drawing
        resource_link<texture> tex;
		tile_size_t left{}, top{};
		// NOTE: this is intentionally not a resource_link
		//		since tiles themselves aren't a resource
		//		and are always created after the tileset they're in
		const tileset *source = nullptr;
	};

	//bad tile can be used as a sentinel
	//only bad tile should ever have a null source
	constexpr auto bad_tile = tile{};

	bool operator==(const tile &lhs, const tile &rhs) noexcept;
	bool operator!=(const tile &lhs, const tile &rhs) noexcept;
	bool operator<(const tile &lhs, const tile &rhs) noexcept;

	struct tileset;
	struct tile_settings;

	namespace detail
	{
		void parse_tiles(tileset& tileset, tile_size_t tile_size, const data::parser_node& n, data::data_manager& d);
		void load_tile_settings(tile_settings& r, data::data_manager& d);
	}

	//contains all the tiles defined by a particular tileset
	//used to map the integers in maps to actual tiles
	struct tileset_t {};
	struct tileset : public resource_type<tileset_t>
	{
		void load(data::data_manager&) override;
		void serialise(const data::data_manager&, data::writer&) const override;

		// as cache of the tile loading data
		struct tile_source
		{
			unique_id texture;
			tile_size_t left, top;
			tile_index_t tiles_per_row;
			tile_index_t tile_count;
		};

		// stored in order of file load
		std::vector<tile_source> tile_source_groups;
		std::vector<tile> tiles; 
		tag_list tags;

	protected:
		// used by terrainsets to have the tile portion of their data serialised
		void serialise_impl(const data::data_manager&, data::writer&) const;

	};

	struct tile_settings_t {};
	struct tile_settings : public::hades::resources::resource_type<tile_settings_t>
	{
		void load(data::data_manager&) override;
		void serialise(const data::data_manager&, data::writer&) const override;

		resource_link<tileset> error_tileset;
		resource_link<tileset> empty_tileset;

		std::vector<resource_link<tileset>> tilesets;
		tile_size_t tile_size = {};
	};

	//exceptions: all three throw resource_error
	// either as resource_null or resource_wrong_type
	const tile_settings *get_tile_settings();
	tile_size_t get_tile_size();
	const tile& get_error_tile();
	[[deprecated]] const tile& get_empty_tile();
	[[nodiscard]] const tile& get_empty_tile(const tile_settings&);

	unique_id get_tile_settings_id() noexcept;
}

//tile_map system
//allows creating and editing tile maps
namespace hades
{
	class tile_error : public runtime_error
	{
	public:
		using runtime_error::runtime_error;
	};

	//exceptions thrown by tile map utilities
	class tile_not_found : public tile_error
	{
	public:
		using tile_error::tile_error;
	};

	class tileset_not_found : public tile_error
	{
	public:
		using tile_error::tile_error;
	};

	constexpr auto bad_tile_index = tile_index_t{ -1 };
	constexpr auto bad_tile_position = tile_position{ -1, -1 };
	// unique id for each tile
	// TODO: use this properly throughout tiles and terrain .hpp/.cpp
	// TODO: this doesn't need the full range of size_type, reduce it to uint16
	using tile_id_t = std::vector<resources::tile>::size_type;

	//stores a tileset and the starting id for that tileset in a map
	using tileset_info = std::tuple<unique_id, tile_id_t>;

	//this is a map that can be written to disk
	struct raw_map
	{
		std::vector<tileset_info> tilesets;
		//tiles are stored as ids based on the tileset size in tileset_info
		//this means that some tilesets may have tiles that cannot be in the map
		// tilesets are effectivly trucated based on the last tile they have in the actual map
		std::vector<tile_id_t> tiles;
		tile_index_t width{};
	};

	//read and write raw maps
	void write_raw_map(const raw_map&, data::writer&);
	raw_map read_raw_map(const data::parser_node&, std::size_t size);

	//this is a map that can be used for game logic
	struct tile_map
	{
		std::vector<const resources::tileset*> tilesets;
		//tiles are stored as full ids according to each tileset
		std::vector<tile_id_t> tiles;
		tile_index_t width{};
	};

	//TODO: make many of these noexcept and constexpr

	// index type is storable in the curve system
	tile_position from_tile_index(tile_index_t, tile_index_t map_width);
	tile_index_t to_tile_index(tile_position, tile_index_t map_width);
	tile_position from_tile_index(tile_index_t, const tile_map&);
	tile_index_t to_tile_index(tile_position, const tile_map&);

	//tile to pixel conversions
	//convert pixel mesurement to tile measurement
	int32 to_tiles(int32 pixels, resources::tile_size_t tile_size) noexcept; 
	//convert pixel position to tilemap position
	tile_position to_tiles(vector2_int pixels, resources::tile_size_t tile_size);
	tile_position to_tiles(vector2_float real_pixels, resources::tile_size_t tile_size);
	//reverse of the two above functions
	int32 to_pixels(int32 tiles, resources::tile_size_t tile_size) noexcept;
	vector2_int to_pixels(tile_position tiles, resources::tile_size_t tile_size) noexcept;

	//throws tile_error, if the tile_map is malformed
	//returns the size of the map expressed in tiles
	tile_position get_size(const raw_map&);
	tile_position get_size(const tile_map&);

	//converts a raw map into a tile map
	// exceptions: tileset_not_found
	tile_map to_tile_map(const raw_map&);
	//the reverse of the above, only throws standard exceptions(eg. bad_alloc)
	raw_map to_raw_map(const tile_map&);
	raw_map to_raw_map(tile_map&&);
	//get tile from raw_map
	// exceptions: tile_not_found, tileset_not_found
	resources::tile get_tile(const raw_map&, tile_id_t);
	//get tile from tile map
	// exceptions: tile_not_found
	resources::tile get_tile(const tile_map&, tile_id_t);

	//gets the tile id in a format appropriate for storing in a raw map
	// exceptions: tileset_not_found and tile_not_found
	tile_id_t get_tile_id(const raw_map&, const resources::tile&);
	//gets the tile id in a format appropriate for storing in tile map
	// exceptions: tile_not_found
	tile_id_t get_tile_id(const tile_map&, const resources::tile&);
	
	const tag_list &get_tags(const resources::tile&);

	//for getting information out of a tile map
	resources::tile get_tile_at(const tile_map&, tile_position);
	const tag_list &get_tags_at(const tile_map&, tile_position);

	// exceptions: tileset_not_found
	[[nodiscard]] tile_map make_map(tile_position size, const resources::tile&, const resources::tile_settings&);

	//set the vectors relative to the current size
	//eg, to expand the map in all directions
	// top_left{-1, -1}, bottom_right = current_size + {1, 1}
	// tiles that fall outside the new size are erased
	// new areas will be set to tile&
	void resize_map_relative(tile_map&, vector2_int top_left, vector2_int bottom_right,
		const resources::tile&, const resources::tile_settings&);
	//as above, new tiles will be set as it tile& was resources::get_empty_tile()
	void resize_map_relative(tile_map&, vector2_int top_left, vector2_int bottom_right, const resources::tile_settings&);
	//as above, places current map content at offset
	void resize_map(tile_map&, vector2_int size, vector2_int offset, const resources::tile&, const resources::tile_settings&);
	//as above, new tiles will be set as if tile& was resources::get_empty_tile()
	void resize_map(tile_map&, vector2_int size, vector2_int offset, const resources::tile_settings&);

	//for editing a tile map
	void place_tile(tile_map&, tile_position, tile_id_t);
	void place_tile(tile_map&, tile_position, const resources::tile&, const resources::tile_settings&);
	//positions outside the map will be ignored
	void place_tile(tile_map&, const std::vector<tile_position>&, tile_id_t);
	void place_tile(tile_map&, const std::vector<tile_position>&, const resources::tile&, const resources::tile_settings&);

	// check if tile is within tilemap
	constexpr bool within_world(tile_position position, tile_position world_size) noexcept;

	//helpers to create a list of positions in a desired shape
	//TODO: move to general game logic
	[[deprecated]] std::vector<tile_position> make_position_square(tile_position position, tile_index_t size);
	[[deprecated]] std::vector<tile_position> make_position_square_from_centre(tile_position middle, tile_index_t half_size);
	[[deprecated]] std::vector<tile_position> make_position_rect(tile_position, tile_position size);
	[[deprecated]] std::vector<tile_position> make_position_circle(tile_position middle, tile_index_t radius);
	[[deprecated]] std::array<tile_position, 9> make_position_9patch(tile_position middle) noexcept;

	// Prefer this over the above functions, as it won't allocate
	// safe versions wont check pass invalid tiles along to Func
	// non-safe versions func needs to handle being passed bad_tile_poistion/bad_tile_index
	template<std::invocable<tile_position> Func>
	void for_each_position_rect(tile_position position, tile_position size,
		tile_position world_size, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_position>);
	template<std::invocable<tile_position> Func>
	void for_each_safe_position_rect(tile_position position, tile_position size,
		tile_position world_size, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_position>);

	template<std::invocable<tile_position> Func>
	void for_each_position_circle(tile_position middle, tile_index_t radius, tile_position world_size, Func &&f) noexcept(std::is_nothrow_invocable_v<Func, tile_position>);
	template<std::invocable<tile_position> Func>
	void for_each_safe_position_circle(tile_position middle, tile_index_t radius, tile_position world_size, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_position>);

	template<std::invocable<tile_position> Func>
	void for_each_position_line(tile_position start, tile_position end, tile_position world_size, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_position>);
	template<std::invocable<tile_position> Func>
	void for_each_safe_position_line(tile_position start, tile_position end, tile_position world_size, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_position>);

	template<std::invocable<tile_position> Func>
	void for_each_position_diagonal(tile_position start, tile_position end, tile_position world_size, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_position>);
	template<std::invocable<tile_position> Func>
	void for_each_safe_position_diagonal(tile_position start, tile_position end, tile_position world_size, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_position>);

	template<std::invocable<tile_index_t> Func>
	void for_each_index_rect(const tile_index_t pos, tile_position size,
		const tile_index_t map_width, const tile_index_t max_index, Func&& f)
		noexcept(std::is_nothrow_invocable_v<Func, tile_index_t>);
	template<std::invocable<tile_index_t> Func>
	void for_each_safe_index_rect(const tile_index_t pos, tile_position size,
		const tile_index_t map_width, const tile_index_t max_index, Func&& f)
		noexcept(std::is_nothrow_invocable_v<Func, tile_index_t>);

	template<std::invocable<tile_index_t> Func>
	void for_each_safe_index_9_patch(const tile_index_t pos,
		const tile_index_t map_width, const tile_index_t max_index, Func&& f)
		noexcept(std::is_nothrow_invocable_v<Func, tile_index_t>);
	template<std::invocable<tile_index_t> Func>
	void for_each_index_9_patch(const tile_index_t pos,
		const tile_index_t map_width, const tile_index_t max_index, Func&& f)
		noexcept(std::is_nothrow_invocable_v<Func, tile_index_t>);
	
	// For each expanding calls func with an expanding ring of cell positions
	//	stopping when func returns true
	enum class for_each_expanding_return : bool {
		stop = true,
		continue_expanding = false
	};

	template<invocable_r<for_each_expanding_return, tile_position> Func>
	[[deprecated]]
	void for_each_safe_expanding_position(const tile_position position,
		const tile_position size, const tile_position world_size, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_position>);
	template<invocable_r<for_each_expanding_return, tile_position> Func>
	[[deprecated]]
	void for_each_expanding_position(const tile_position position,
		const tile_position size, const tile_position world_size, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_position>);

	template<invocable_r<for_each_expanding_return, tile_index_t> Func>
	void for_each_safe_expanding_index(const tile_index_t pos, const tile_index_t map_width,
		const tile_index_t max_index, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_index_t>);
	template<invocable_r<for_each_expanding_return, tile_index_t> Func>
	void for_each_expanding_index(const tile_index_t pos, const tile_index_t map_width,
		const tile_index_t max_index, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_index_t>);
}

#include "hades/detail/tiles.inl"

#endif // !HADES_TILES_HPP
