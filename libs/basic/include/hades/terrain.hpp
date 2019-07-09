#ifndef HADES_TERRAIN_HPP
#define HADES_TERRAIN_HPP

#include "hades/resource_base.hpp"

#include "hades/data.hpp"
#include "hades/rectangle_math.hpp"
#include "hades/tiles.hpp"

//tiles namespace is for the terrain editing and rendering subsystem
namespace hades
{
	void register_terrain_resources(data::data_manager&);
	void register_terrain_resources(data::data_manager&, detail::find_make_texture_f);
}

namespace hades::resources
{
	//NOTE: see http://www.cr31.co.uk/stagecast/wang/2corn.html
	//values for 2-corner transitions
	//named based on which tile corners hold a terrain
	// other corners are empty
	//the order is important for the algorithm to work
	enum transition_tile_type : std::size_t {
		transition_begin,
		none = transition_begin,
		top_right,
		bottom_right,
		top_right_bottom_right,
		bottom_left,
		top_right_bottom_left,
		bottom_left_right,
		top_right_bottom_left_right,
		top_left,
		top_left_right,
		top_left_bottom_right,
		top_left_right_bottom_right,
		top_left_bottom_left,
		top_left_right_bottom_left,
		top_left_bottom_left_right,
		all,
		transition_end
	};

	struct terrain final : tileset
	{
		terrain();

		//an array of tiles for each transition_type, except all(which should be an empty tile)
		//use get_transitions() to access the correct element
		std::array<std::vector<tile>, all> terrain_transition_tiles;

		//NOTE: also has access to std::vector<tile> tiles
		// which contains all the tiles from the above lists as well
	};

	struct terrainset_t final {};

	struct terrainset final : resource_type<terrainset_t>
	{
		terrainset();

		std::vector<const terrain*> terrains;
	};

	struct terrain_settings_t final {};

	struct terrain_settings final : tile_settings
	{
		terrain_settings();

		const terrain *empty_terrain = nullptr;
		const terrainset* empty_terrainset = nullptr;
		std::vector<const terrain*> terrains;
		std::vector<const terrainset*> terrainsets;
	};

	const terrain_settings *get_terrain_settings();
	const terrain *get_terrain(const resources::tile&);

	std::string_view get_empty_terrainset_name() noexcept;
	const terrain *get_empty_terrain();
	//NOTE: used for maps with no tile terrains
	// returns a terrainset only holding the empty terrain
	const terrainset* get_empty_terrainset();
	
	std::vector<tile> &get_transitions(terrain&, transition_tile_type);
	const std::vector<tile> &get_transitions(const terrain&, transition_tile_type);

	tile get_random_tile(const terrain&, transition_tile_type);
}

namespace hades
{
	class terrain_error : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	using terrain_count_t = tile_count_t;

	struct raw_terrain_map
	{
		unique_id terrainset;
		//terrain vertex are indexed starting at 1
		// 0 is reserved for the empty vertex
		//if empty, then should be filled with empty vertex
		std::vector<terrain_count_t> terrain_vertex;

		//if empty, then should be generated
		std::vector<raw_map> terrain_layers;

		raw_map tile_layer;
	};

	bool is_valid(const raw_terrain_map&);
	bool is_valid(const raw_terrain_map&, vector_int level_size, resources::tile_size_t tile_size);

	//NOTE: doesn't write the tile layer, this must be done seperately
	//		with write_raw_map(m.tile_layer, ...);
	void write_raw_terrain_map(const raw_terrain_map &m, data::writer &w);
	std::tuple<unique_id, std::vector<terrain_count_t>, std::vector<raw_map>>
		read_raw_terrain_map(const data::parser_node &p);

	struct terrain_map
	{
		//a terrainset lists the terrain types that can be used in a level
		const resources::terrainset *terrainset = nullptr;

		//vertex of terrain
		std::vector<const resources::terrain*> terrain_vertex;
		std::vector<tile_map> terrain_layers;
	
		tile_map tile_layer;
	};

	//converts a raw map into a tile map
	// exceptions: tileset_not_found, terrain_error
	terrain_map to_terrain_map(const raw_terrain_map&);
	//the reverse of the above, only throws standard exceptions(eg. bad_alloc)
	raw_terrain_map to_raw_terrain_map(const terrain_map&);

	//type for positioning vertex in a terrain map
	using terrain_vertex_position = tile_position;

	terrain_map make_map(tile_position size, const resources::terrainset*, const resources::terrain*);

	terrain_count_t get_width(const terrain_map&);
	terrain_vertex_position get_size(const terrain_map&);

	bool within_map(terrain_vertex_position map_size, terrain_vertex_position position);
	bool within_map(const terrain_map&, terrain_vertex_position position);

	//index tile_corners using rect_corners from math.hpp
	using tile_corners = std::array<const resources::terrain*, 4u>;
	const resources::terrain *get_corner(const tile_corners&, rect_corners);

	//pass a array indicating which corners have terrain in them
	//the array should be indexed according to rect_corners in math.hpp
	resources::transition_tile_type get_transition_type(const std::array<bool, 4u>&);
	//Iter1 and Iter 2 must point to terrain*
	template<typename  Iter1, typename Iter2>
	resources::transition_tile_type get_transition_type(tile_corners, Iter1 begin, Iter2 end);

	tile_corners get_terrain_at_tile(const terrain_map&, tile_position);
	const resources::terrain *get_vertex(const terrain_map&, terrain_vertex_position);

	//NOTE: the result of this function may contain duplicates
	tag_list get_tags_at(const terrain_map&, tile_position);

	//set the vectors relative to the current size
	//eg, to expand the map in all directions
	// top_left{-1, -1}, bottom_right = current_size + {1, 1}
	// tiles that fall outside the new size are erased
	// new areas will be set to tile&
	void resize__map_relative(terrain_map&, vector_int top_left, vector_int bottom_right,
		const resources::terrain&);
	//as above, new tiles will be set as it tile& was resources::get_empty_tile()
	void resize_map_relative(terrain_map&, vector_int top_left, vector_int bottom_right);
	//as above, places current map content at offset
	void resize_map(terrain_map&, vector_int size, vector_int offset, const resources::terrain&);
	//as above, new tiles will be set as it tile& was resources::get_empty_tile()
	void resize_map(terrain_map&, vector_int size, vector_int offset);

	std::vector<tile_position> get_adjacent_tiles(terrain_vertex_position);
	std::vector<tile_position> get_adjacent_tiles(const std::vector<terrain_vertex_position>&);

	//for editing a terrain map
	//use the make_position_* functions from tiles.hpp
	void place_tile(terrain_map&, tile_position, const resources::tile&);
	//positions outside the map will be ignored
	void place_tile(terrain_map&, const std::vector<tile_position>&, const resources::tile&);

	void place_terrain(terrain_map&, terrain_vertex_position, const resources::terrain*);
	//positions outside the map will be ignored
	void place_terrain(terrain_map&, const std::vector<terrain_vertex_position>&, const resources::terrain*);
}

#include "hades/detail/terrain.inl"

#endif // !HADES_TERRAIN_HPP
