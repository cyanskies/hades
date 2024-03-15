#ifndef HADES_TERRAIN_HPP
#define HADES_TERRAIN_HPP

#include <variant>

#include "hades/resource_base.hpp"

#include "hades/data.hpp"
#include "hades/rectangle_math.hpp"
#include "hades/tiles.hpp"

//tiles namespace is for the terrain editing and rendering subsystem
namespace hades
{
	void register_terrain_resources(data::data_manager&);
	void register_terrain_resources(data::data_manager&, detail::make_texture_link_f);
}

namespace hades::resources
{
	//NOTE: see http://www.cr31.co.uk/stagecast/wang/2corn.html
	//values for 2-corner transitions
	//named based on which tile corners hold a terrain
	// other corners are empty
	//the order is important for the algorithm to work
	enum class transition_tile_type : uint8 {
		none, 
		transition_begin = none,
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
		void load(data::data_manager&) final override;
		void serialise(const data::data_manager&, data::writer&) const override;

		struct terrain_source final : public tileset::tile_source
		{
			enum class layout_type
			{
				empty,
				war3,
				custom
			};

			using stored_layout = std::variant<layout_type, std::vector<transition_tile_type>>;
			stored_layout layout;
		};

		std::vector<terrain_source> terrain_source_groups;

		//an array of tiles for each transition_type, except all(which should be an empty tile)
		//use get_transitions() to access the correct element
		std::array<std::vector<tile>, enum_type(transition_tile_type::all)> terrain_transition_tiles;

		//NOTE: also has access to std::vector<tile> tiles
		// which contains all the tiles from the above lists as well
	};

	struct terrainset_t {};

	struct terrainset final : resource_type<terrainset_t>
	{
		void load(data::data_manager&) final override;
		void serialise(const data::data_manager&, data::writer&) const override;

		std::vector<resource_link<terrain>> terrains;
		resource_link<terrain> cliff_terrain;
	};

	struct terrain_settings_t {};

	struct terrain_settings final : tile_settings
	{
		void load(data::data_manager&) final override;
		void serialise(const data::data_manager&, data::writer&) const final override;

		resource_link<terrain> editor_terrain = {};
		resource_link<terrain> grid_terrain = {};
		resource_link<terrain> empty_terrain = {};
		resource_link<terrainset> empty_terrainset = {};
		resource_link<terrainset> editor_terrainset = {};
		vector3<float> sun_direction = {};
		// TODO: undeprecate, draw the background whereever this terrain is
		[[deprecated]] resource_link<terrain> background_terrain = {};
		std::vector<resource_link<terrain>> terrains;
		std::vector<resource_link<terrainset>> terrainsets;
		std::uint8_t height_default = 100;
		std::uint8_t height_min = 0; // TODO: shadows seem to break on terrain height 0, needs investigation
		std::uint8_t height_max = std::numeric_limits<std::uint8_t>::max();
	};

	unique_id get_background_terrain_id() noexcept;

	const terrain_settings *get_terrain_settings();
	const terrain *get_terrain(const resources::tile&);

	std::string_view get_empty_terrainset_name() noexcept;

	// exceptions: these three can throw resource_error
	//const terrain *get_empty_terrain();
	[[nodiscard]] const terrain* get_empty_terrain(const terrain_settings&);
	[[deprecated]] const terrain* get_background_terrain();
	//NOTE: used for maps with no tile terrains
	// returns a terrainset only holding the empty terrain
	[[deprecated]] const terrainset* get_empty_terrainset();
	
	std::vector<tile> &get_transitions(terrain&, transition_tile_type, const resources::terrain_settings&);
	const std::vector<tile> &get_transitions(const terrain&, transition_tile_type, const resources::terrain_settings&);

	tile get_random_tile(const terrain&, transition_tile_type, const resources::terrain_settings&);
}

namespace hades
{
	class terrain_error : public runtime_error
	{
	public:
		using runtime_error::runtime_error;
	};

	//thrown if the map cannot be converted or loaded
	//the map will need terrain layers to be added or removed
	class terrain_layers_error : public terrain_error
	{
	public:
		using terrain_error::terrain_error;
	};

	using terrain_index_t = tile_index_t;
	using terrain_id_t = tile_id_t;

	struct raw_terrain_map
	{
		// TODO: remove
		enum class cliff_type : std::uint8_t {
			none = 0b0000000, 
			middle = 0b0000001, // middle indicates both vertex are in middle of cliff
			end_top_left = 0b0000010, // top is for right cliffs, left otherwise
			end_bottom_right = 0b0000011, // bottom is for right cliffs, right otherwise
		};

		struct cliff_info
		{
			bool triangle_type : 1,
				diag : 2, // TODO: we only need 1 bit for these
				bottom : 2,
				right : 2;
		};

		raw_map tile_layer;
		raw_map cliffs;

		//terrain vertex are id starting at 1
		// 0 is reserved for the empty vertex
		//if empty, then should be filled with empty vertex
		std::vector<terrain_id_t> terrain_vertex;
		std::vector<std::uint8_t> heightmap;
		std::vector<cliff_info> cliff_data;

		//if empty, then should be generated
		std::vector<raw_map> terrain_layers;

		unique_id terrainset;

	};

	bool is_valid(const raw_terrain_map&);
	bool is_valid(const raw_terrain_map&, vector2_int level_size, resources::tile_size_t tile_size);

	void write_raw_terrain_map(const raw_terrain_map &m, data::writer &w);
	
	raw_terrain_map read_raw_terrain_map(const data::parser_node &p, std::size_t layer_size, std::size_t vert_size);

	struct terrain_map
	{
		// TODO: this is clockwise triangle ordering (opengl uses counter clockwise by convention)
		//			we should change this in case we ever encounter face culling issues
		// triangle types:
		// quad is made of two triangles starting with the triangle that
		// defines the left edge of the quad, so each quad has a left and right triangle
		// for uphill:
		//		triangle 1:
		//				1: top-left
		//				2: top-right
		//				3: bottom-left
		//		triangle 2:
		//				4: top-right
		//				5: bottom-right
		//				6: bottom-left
		//
		// for downhill:
		//		triangle 1:
		//				1: top-left
		//				2: bottom-right
		//				3: bottom-left
		//		triangle 2:
		//				4: top-left
		//				5: top-right
		//				6: bottom-right
		static constexpr auto triangle_downhill = true;
		static constexpr auto triangle_uphill = false;
		static constexpr auto triangle_default = triangle_uphill;
		static constexpr auto triangle_left = true;
		static constexpr auto triangle_right = false;

		enum class cliff_layer_layout : std::uint8_t {
			surface, diag, right, bottom
		};

		using cliff_info = raw_terrain_map::cliff_info;

		tile_map tile_layer;
		// should be 4 times the size of tile_layer ( surface tile, diagonal tile, right tile, bottom tile )
		tile_map cliffs;

		//vertex of terrain
		std::vector<const resources::terrain*> terrain_vertex;
		// tile vertex of height (6 vertex per tile)
		std::vector<std::uint8_t> heightmap;
		std::vector<cliff_info> cliff_data; // per tile

		// TODO: sun_angle

		// tiles exist between every 4 vertex
		std::vector<tile_map> terrain_layers;
	
		//a terrainset lists the terrain types that can be used in a level
		const resources::terrainset* terrainset = nullptr;
	};

	// converts triangle pair indexes [0-5] to equivilent quad indexes [0-3]
	constexpr rect_corners triangle_index_to_quad_index(std::uint8_t, bool tri_type) noexcept;
	// reverse of above
	constexpr auto bad_triangle_index = std::numeric_limits<std::uint8_t>::max();
	// second element may be bad_triangle_index
	constexpr std::pair<std::uint8_t, std::uint8_t> quad_index_to_triangle_index(rect_corners, bool tri_type) noexcept;
	
	//converts a raw map into a tile map
	// exceptions: tileset_not_found, terrain_error, terrain_layers_error
	terrain_map to_terrain_map(const raw_terrain_map&);
	terrain_map to_terrain_map(raw_terrain_map&&);
	//the reverse of the above, only throws standard exceptions(eg. bad_alloc)
	raw_terrain_map to_raw_terrain_map(const terrain_map&);
	raw_terrain_map to_raw_terrain_map(terrain_map&&);

	//type for positioning vertex in a terrain map
	using terrain_vertex_position = tile_position;

	terrain_map make_map(tile_position size, const resources::terrainset*, const resources::terrain*, const resources::terrain_settings&);

	terrain_index_t get_width(const terrain_map&);
	//returns the size of the map expressed in tiles
	tile_position get_size(const terrain_map&);
	// size of the map expressed in vertex
	terrain_vertex_position get_vertex_size(const terrain_map&);
	// throws: terrain_error if map sizes differ
	void copy_heightmap(terrain_map& target, const terrain_map& src);

	// index the array using rectangle_math.hpp::hades::rect_corners
	[[nodiscard]]
	std::array<terrain_index_t, 4> to_terrain_index(tile_position tile_index , terrain_index_t map_width) noexcept;
	
	inline tile_position from_tile_index(tile_index_t i, const terrain_map& t) noexcept
	{
		return from_tile_index(i, t.tile_layer);
	}

	inline tile_index_t to_tile_index(tile_position p, const terrain_map& t) noexcept
	{
		return to_tile_index(p, t.tile_layer);
	}

	// check if vertex is within map
	bool within_map(terrain_vertex_position map_size, terrain_vertex_position position) noexcept;
	// check if vertex is within map
	bool within_map(const terrain_map&, terrain_vertex_position position);

	bool edge_of_map(terrain_vertex_position map_size, terrain_vertex_position position) noexcept;

	//index tile_corners using rect_corners from math.hpp
	using tile_corners = std::array<const resources::terrain*, 4u>;
	const resources::terrain *get_corner(const tile_corners&, rect_corners) noexcept;

	//pass a array indicating which corners have terrain in them
	//the array should be indexed according to rect_corners in math.hpp
	resources::transition_tile_type get_transition_type(const std::array<bool, 4u>&) noexcept;
	//Iter1 and Iter 2 must point to terrain*
	template<typename  Iter1, typename Iter2>
	resources::transition_tile_type get_transition_type(tile_corners, Iter1 begin, Iter2 end) noexcept;

	tile_corners get_terrain_at_tile(const terrain_map&, tile_position, const resources::terrain_settings&);

	struct triangle_height_data
	{
		// contains height for both triangles
		std::array<std::uint8_t, 6> height;
		bool triangle_type;
	};
	// returns height arranged into two triangles
	triangle_height_data get_height_for_triangles(tile_position, const terrain_map&);
	void set_height_for_triangles(tile_position, terrain_map&, triangle_height_data); // TODO: temp remove
	// index the array using rectangle_math.hpp::hades::rect_corners
	// returns the height in the 4 corners of the tile
	std::array<std::uint8_t, 4> get_max_height_in_corners(tile_position tile_index, const terrain_map& map);
	std::array<std::uint8_t, 4> get_max_height_in_corners(const triangle_height_data&) noexcept;
	// returns the height at both edges of an edge (left/top first)
	std::array<std::uint8_t, 2> get_height_for_top_edge(const triangle_height_data&) noexcept;
	std::array<std::uint8_t, 2> get_height_for_left_edge(const triangle_height_data&) noexcept;
	std::array<std::uint8_t, 2> get_height_for_right_edge(const triangle_height_data&) noexcept;
	std::array<std::uint8_t, 2> get_height_for_bottom_edge(const triangle_height_data&) noexcept;
	// as above, but left triangle first and right triangle second
	std::array<std::uint8_t, 4> get_height_for_diag_edge(const triangle_height_data&) noexcept;

	// NOTE: cliffs are *owned* by the tile they are attached too
	//		eg: a tile owns cliffs that pass through its middle, and
	//			cliffs to it's right and bottom
	// This info is used mostly for rendering
	terrain_map::cliff_info get_cliff_info(tile_index_t, const terrain_map&) noexcept;
	terrain_map::cliff_info get_cliff_info(tile_position, const terrain_map&) noexcept;
	void set_cliff_info_tmp(tile_position, terrain_map&, terrain_map::cliff_info); // TODO: temp remove

	// return all adjacent cliffs to this tile
	// index array using rectangle_math.hpp::hades::rect_edges
	std::array<bool, 4> get_adjacent_cliffs(tile_position, const terrain_map&) noexcept;
	// index array using rectangle_math.hpp::hades::rect_corners
	std::array<bool, 4> get_cliffs_corners(const tile_position p, const terrain_map& m) noexcept;

	// returns true if a cliff splits a tile accross the middle(between the two triangles that make up a tile quad)
	bool is_tile_split(tile_position, const terrain_map& map);

	const resources::terrain *get_vertex(const terrain_map&, terrain_vertex_position);

	//NOTE: the result of this function may contain duplicates
	tag_list get_tags_at(const terrain_map&, tile_position);

	//set the vectors relative to the current size
	//eg, to expand the map in all directions
	// top_left{-1, -1}, bottom_right = current_size + {1, 1}
	// tiles that fall outside the new size are erased
	// new areas will be set to terrain*
	// TODO: replace vector2_int with tile_position
	void resize_map_relative(terrain_map&, vector2_int top_left, vector2_int bottom_right,
		const resources::terrain*, std::uint8_t height, const resources::terrain_settings&);
	//as above, new tiles will be set as it tile& was resources::get_empty_tile()
	[[deprecated]] void resize_map_relative(terrain_map&, vector2_int top_left, vector2_int bottom_right);
	//as above, places current map content at offset
	void resize_map(terrain_map&, vector2_int size, vector2_int offset, const resources::terrain*,
		std::uint8_t height, const resources::terrain_settings& s);
	//as above, new tiles will be set as if tile& was resources::get_empty_tile()
	[[deprecated]] void resize_map(terrain_map&, vector2_int size, vector2_int offset);

	template<std::invocable<tile_position> Func>
	void for_each_adjacent_tile(terrain_vertex_position, const terrain_map&, Func&& f) noexcept;
	template<std::invocable<tile_position> Func>
	void for_each_safe_adjacent_tile(terrain_vertex_position, const terrain_map&, Func&& f) noexcept;

	//for editing a terrain map
	//use the make_position_* functions from tiles.hpp
	void place_tile(terrain_map&, tile_position, const resources::tile&, const resources::terrain_settings&);
	//positions outside the map will be ignored
	void place_tile(terrain_map&, const std::vector<tile_position>&, const resources::tile&, const resources::terrain_settings&);

	void place_terrain(terrain_map&, terrain_vertex_position, const resources::terrain*, const resources::terrain_settings&);
	//positions outside the map will be ignored
	void place_terrain(terrain_map&, const std::vector<terrain_vertex_position>&, const resources::terrain*, const resources::terrain_settings&);

	template<std::invocable<std::uint8_t> Func>
		requires std::same_as<std::invoke_result_t<Func, std::uint8_t>, std::uint8_t>
	void change_terrain_height(terrain_map&, terrain_vertex_position, Func&&);
	
	// for a particular vertex, calls the appropriate number of Func calls to fix up the height
	template<typename Func>
	void for_each_height_vertex(tile_position, rect_corners, tile_position world_size, Func&&);

	// TODO: need to fix for cliffs
	// TODO: must target a tile so that it can disabiguate when a cliff is present
	//		these are fine, they can just fail for cliff areas
	//		Need to make a new one that picks vertex for a tile
	void raise_terrain(terrain_map&, terrain_vertex_position, std::uint8_t, const resources::terrain_settings*);
	void lower_terrain(terrain_map&, terrain_vertex_position, std::uint8_t, const resources::terrain_settings*);
	void set_height_at(terrain_map&, terrain_vertex_position, std::uint8_t, const resources::terrain_settings*);

	void raise_terrain(terrain_map&, tile_position, bool triangle, rect_corners, std::uint8_t, const resources::terrain_settings*);

	void swap_triangle_type(terrain_map&, tile_position);

	bool is_cliff(const terrain_map&, tile_position);

	// returns true if add cliff would work on this edge
	bool can_add_cliff(const terrain_map&, tile_position, rect_edges);
	// add a cliff in this position if possible
	void add_cliff();
	// remove a cliff from this position, if present
	void remove_cliff();

	// project 'p' onto the flat version of 'map'
	world_vector_t project_onto_terrain(world_vector_t p, float rot_degrees,
		const resources::tile_size_t tile_size, const terrain_map& map);
}

#include "hades/detail/terrain.inl"

#endif // !HADES_TERRAIN_HPP
