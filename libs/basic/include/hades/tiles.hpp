#ifndef HADES_TILES_HPP
#define HADES_TILES_HPP

#include "hades/exceptions.hpp"
#include "hades/game_types.hpp"
#include "hades/math.hpp"
#include "hades/parser.hpp"
#include "hades/resource_base.hpp"
#include "hades/vector_math.hpp"
#include "hades/writer.hpp"

// system for working with logical tiles
// editing a tilemap, and getting tag_lists from the map

namespace hades::resources
{
	//fwd declaration
	// tiles cannot be drawn unless linked against hades-core
	// which defines textures
	struct texture;
}

namespace hades::detail
{
	using find_make_texture_f = resources::texture* (*)(data::data_manager&, unique_id, unique_id);
}

namespace hades
{
	//NOTE:this doesn't register the texture resource
	// this is to allow logical gameplay tilemap usage
	// without bringing in graphics resources and linkage
	void register_tiles_resources(data::data_manager&);
	//as above, but loads textures from the requested function
	void register_tiles_resources(data::data_manager&, detail::find_make_texture_f);
}

//resources for loading tiles and tilesets
namespace hades::resources
{
	//maximum tile size is capped by texture size
	using tile_size_t = texture_size_t;

	struct tile
	{
		//texture and [left, top] are used for drawing
		const resources::texture *texture = nullptr;
		tile_size_t left{}, top{};
		//used for game logic
		tag_list tags{};
	};

	bool operator==(const tile &lhs, const tile &rhs);
	bool operator!=(const tile &lhs, const tile &rhs);
	bool operator<(const tile &lhs, const tile &rhs);

	//contains all the tiles defined by a particular tileset
	//used to map the integers in maps to actual tiles
	struct tileset_t {};
	struct tileset : public resource_type<tileset_t>
	{
		tileset();

		std::vector<tile> tiles;
	};

	struct tile_settings_t {};
	struct tile_settings : public::hades::resources::resource_type<tile_settings_t>
	{
		tile_settings();

		tile_size_t tile_size{};
		const tileset *error_tileset = nullptr;
		const tileset *empty_tileset = nullptr;

		std::vector<const tileset*> tilesets;
	};

	//exceptions: all three throw resource_error
	// either as resource_null or resource_wrong_type
	const tile_settings *get_tile_settings();
	const tile &get_error_tile();
	const tile &get_empty_tile();

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

	//stores a count of tiles, or a tiles index
	using tile_count_t = std::vector<resources::tile>::size_type;

	//stores a tileset and the starting id for that tileset in a map
	using tileset_info = std::tuple<unique_id, tile_count_t>;

	//this is a map that can be written to disk
	struct raw_map
	{
		std::vector<tileset_info> tilesets;
		//tiles are stored as ids based on the tileset size in tileset_info
		std::vector<tile_count_t> tiles;
		tile_count_t width;
	};

	//read and write raw maps
	void write_raw_map(const raw_map&, data::writer&);
	raw_map read_raw_map(const data::parser_node&);

	//this is a map that can be used for game logic
	struct tile_map
	{
		std::vector<const resources::tileset*> tilesets;
		//tiles are stored as full ids according to each tileset
		std::vector<tile_count_t> tiles;
		tile_count_t width;
	};

	//throws tile_error, if the tile_map is malformed
	tile_position get_size(const tile_map&);

	//converts a raw map into a tile map
	// exceptions: tileset_not_found
	tile_map to_tile_map(const raw_map&);
	//the reverse of the above, only throws standard exceptions(eg. bad_alloc)
	raw_map to_raw_map(const tile_map&);

	//returns nullptr on failure
	const resources::tileset *get_parent_tileset(const resources::tile&);

	//get tile from raw_map
	// exceptions: tile_not_found, tileset_not_found
	const resources::tile& get_tile(const raw_map&, tile_count_t);
	//get tile from tile map
	// exceptions: tile_not_found
	const resources::tile& get_tile(const tile_map&, tile_count_t);

	//gets the tile id in a format appropriate for storing in a raw map
	// exceptions: tileset_not_found and tile_not_found
	tile_count_t get_tile_id(const raw_map&, const resources::tile&);
	//gets the tile id in a format appropriate for storing in tile map
	// exceptions: tile_not_found
	tile_count_t get_tile_id(const tile_map&, const resources::tile&);
	//returns a tile id usable for that tile_map, adds the needed tileset to the map
	// exceptions: tileset_not_found
	tile_count_t make_tile_id(tile_map&, const resources::tile&);

	//for getting information out of a tile map
	using tile_position = vector_int;
	const resources::tile& get_tile_at(const tile_map&, tile_position);
	const tag_list &get_tags_at(const tile_map&, tile_position);

	// exceptions: tileset_not_found
	tile_map make_map(tile_position size, const resources::tile&);

	//set the vectors relative to the current size
	//eg, to expand the map in all directions
	// top_left{-1, -1}, bottom_right = current_size + {1, 1}
	// tiles that fall outside the new size are erased
	// new areas will be set to tile&
	void resize_map_relative(tile_map&, vector_int top_left, vector_int bottom_right,
		const resources::tile&);
	//as above, new tiles will be set as it tile& was resources::get_empty_tile()
	void resize_map_relative(tile_map&, vector_int top_left, vector_int bottom_right);
	//as above, places current map content at offset
	void resize_map(tile_map&, vector_int size, vector_int offset, const resources::tile&);
	//as above, new tiles will be set as it tile& was resources::get_empty_tile()
	void resize_map(tile_map&, vector_int size, vector_int offset);

	//for editing a tile map
	void place_tile(tile_map&, tile_position, tile_count_t);
	void place_tile(tile_map&, tile_position, const resources::tile&);
	//positions outside the map will be ignored
	void place_tile(tile_map&, const std::vector<tile_position>&, tile_count_t);
	void place_tile(tile_map&, const std::vector<tile_position>&, const resources::tile&);

	//helpers to create a list of positions in a desired shape
	//TODO: move to general game logic
	std::vector<tile_position> make_position_square(tile_position position, tile_count_t size);
	std::vector<tile_position> make_position_square_from_centre(tile_position middle, tile_count_t half_size);

	std::vector<tile_position> make_position_rect(tile_position, tile_position size);

	std::vector<tile_position> make_position_circle(tile_position middle, tile_count_t radius);
}

#endif // !HADES_TILES_HPP
