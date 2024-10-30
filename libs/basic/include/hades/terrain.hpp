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

		// TODO: these aren't call loaded or searialise
		resource_link<terrain> editor_terrain = {};
		resource_link<terrain> grid_terrain = {};
		resource_link<terrain> empty_terrain = {};
		resource_link<terrainset> empty_terrainset = {};
		resource_link<terrainset> editor_terrainset = {};
		
		// TODO: undeprecate, draw the background whereever this terrain is
		[[deprecated]] resource_link<terrain> background_terrain = {};
		std::vector<resource_link<terrain>> terrains;
		std::vector<resource_link<terrainset>> terrainsets;
		// NOTE: cliff_max * cliff_height + height_max must be less the uint8_t max (255)
		std::uint8_t height_default = 50;
		std::uint8_t height_min = 1; // TODO: shadows seem to break on terrain height 0, needs investigation
		std::uint8_t height_max = 130;
		std::uint8_t cliff_default = 5;
		std::uint8_t cliff_min = 1; // minimum height of a cliff(to prevent invisible cliffs)
		std::uint8_t cliff_max = 12;
		std::uint8_t cliff_height = 10;
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
		struct [[deprecated]] cliff_info
		{
			bool triangle_type : 1,
				diag : 1,
				bottom : 1,
				right : 1,
				diag_uphill : 1,
				diag_downhill : 1;
		};

		[[deprecated]] raw_map tile_layer;
		[[deprecated]] raw_map cliffs;

		//terrain vertex are id starting at 1
		// 0 is reserved for the empty vertex
		//if empty, then should be filled with empty vertex
		std::vector<terrain_id_t> terrain_vertex;
		std::vector<std::uint8_t> heightmap;
		std::vector<std::uint8_t> cliff_layer;
		[[deprecated]] std::vector<cliff_info> cliff_data;

		//if empty, then should be generated
		std::vector<raw_map> terrain_layers;
		unique_id terrainset;
		// terrain_vertex width
		terrain_index_t width = {};
	};

	bool is_valid(const raw_terrain_map&);
	bool is_valid(const raw_terrain_map&, vector2_int level_size, resources::tile_size_t tile_size);

	void write_raw_terrain_map(const raw_terrain_map &m, data::writer &w);
	
	raw_terrain_map read_raw_terrain_map(const data::parser_node &p, std::size_t layer_size, std::size_t vert_size);

	struct terrain_map
	{
		// triangle types:
		// quad is made of two triangles starting with the triangle that
		// defines the left edge of the quad, so each quad has a left and right triangle
		// for uphill:
		//		triangle 0:
		//				0: top-left
		//				1: bottom-left
		//				2: top-right
		//		triangle 1:
		//				0: top-right
		//				1: bottom-left
		//				2: bottom-right
		//
		// for downhill:
		//		triangle 0:
		//				0: top-left
		//				1: bottom-left
		//				2: bottom-right
		//		triangle 1:
		//				0: top-left
		//				1: bottom-right
		//				2: top-right
		
		// TODO: these should be an enum : bool
		// triangle_type and selected_triangle
		enum class triangle_type : bool {
			triangle_downhill = true,
			triangle_uphill = false,
			triangle_default = triangle_uphill
		};
		static constexpr auto triangle_downhill = triangle_type::triangle_downhill;
		static constexpr auto triangle_uphill = triangle_type::triangle_uphill;
		static constexpr auto triangle_default = triangle_type::triangle_default;
		static constexpr auto triangle_left = true;
		static constexpr auto triangle_right = false;

		// Used to index into the cliff tile layer.
		enum class cliff_layer_layout : std::uint8_t {
			surface, diag, right, bottom
		};

		using cliff_info = raw_terrain_map::cliff_info;

		// TODO: maybe deprecate
		[[deprecated]] tile_map tile_layer;
		// Should be 4 times the size of tile_layer ( surface tile, diagonal tile, right tile, bottom tile )
		// See: cliff_layer_layout
		[[deprecated]] tile_map cliffs;

		// Terrain vertex info.
		// Same size as tile_layer, but with an extra column and row.
		std::vector<const resources::terrain*> terrain_vertex;
		// Tile vertex of height (6 vertex per tile)
		// Since adjacent tiles don't share vertex height,
		// this is 6 times the size of tile_layer
		std::vector<std::uint8_t> heightmap;
		// Cliff layer per tile
		std::vector<std::uint8_t> cliff_layer;
		// TODO: water level std::vector<std::uint8_t> water_level;
		// Cliff info is stored per tile.
		// Same size as tile_layer
		[[deprecated]] std::vector<cliff_info> cliff_data;

		// TODO: sun_angle

		// Dynamically generated tile layers to represent the terrain vertex
		// Each is the same size as tile_layer
		std::vector<tile_map> terrain_layers;
	
		//a terrainset lists the terrain types that can be used in a level
		const resources::terrainset* terrainset = nullptr;
		// terrain_vertex width
		terrain_index_t width = {};
	};

	// converts triangle pair indexes [0-5] to equivilent quad indexes [0-3]
	constexpr rect_corners triangle_index_to_quad_index(std::uint8_t, terrain_map::triangle_type tri_type) noexcept;
	// reverse of above
	constexpr auto bad_triangle_index = std::numeric_limits<std::uint8_t>::max();
	// Return: first is the specific vertex in left triangle, seconds is in right triangle
	//			if a specific vertex doesn't exist in the triangle, the element will be bad_triangle_index
	constexpr std::pair<std::uint8_t, std::uint8_t> quad_index_to_triangle_index(rect_corners, terrain_map::triangle_type tri_type) noexcept;
	
	// converts a raw map into a tile map
	// exceptions: tileset_not_found, terrain_error, terrain_layers_error
	terrain_map to_terrain_map(const raw_terrain_map&, const resources::terrain_settings&);
	terrain_map to_terrain_map(raw_terrain_map&&, const resources::terrain_settings&);
	// the reverse of the above, only throws standard exceptions(eg. bad_alloc)
	raw_terrain_map to_raw_terrain_map(const terrain_map&, const resources::terrain_settings&);
	raw_terrain_map to_raw_terrain_map(terrain_map&&, const resources::terrain_settings&);

	// type for positioning vertex in a terrain map
	using terrain_vertex_position = tile_position;

	// generate a map filled with the supplied terrain
	terrain_map make_map(tile_position size, const resources::terrainset*, const resources::terrain*, const resources::terrain_settings&);

	// returns the row length of the terrain vertex data
	terrain_index_t get_width(const terrain_map&) noexcept;
	// returns the size of the map expressed in tiles
	tile_position get_size(const terrain_map&);
	// size of the map expressed in vertex
	terrain_vertex_position get_vertex_size(const terrain_map&);
	// throws: terrain_error if map sizes differ
	void copy_heightmap(terrain_map& target, const terrain_map& src);

	// get the vertex position of the corner of a tile
	[[nodiscard]]
	constexpr terrain_vertex_position to_vertex_position(tile_position, rect_corners) noexcept;

	inline tile_position from_tile_index(tile_index_t i, const terrain_map& t) noexcept
	{
		return from_tile_index(i, t.width - 1);
	}

	inline tile_index_t to_tile_index(tile_position p, const terrain_map& t)
	{
		return to_tile_index(p, t.width - 1);
	}

	inline terrain_index_t to_vertex_index(terrain_vertex_position p, const terrain_map& t)
	{
		return to_tile_index(p, t.width);
	}

	// check if vertex is within map
	bool within_map(terrain_vertex_position map_size, terrain_vertex_position position) noexcept;
	// check if vertex is within map
	bool within_map(const terrain_map&, terrain_vertex_position position);
	// returns true if this vertex is along the edge of the map
	// ie. the vertex borders less than 4 tiles
	bool edge_of_map(terrain_vertex_position map_size, terrain_vertex_position position) noexcept;

	//index tile_corners using rect_corners from math.hpp
	using tile_corners = std::array<const resources::terrain*, 4u>;
	const resources::terrain *get_corner(const tile_corners&, rect_corners) noexcept;

	//pass a array indicating which corners have terrain in them
	//the array should be indexed according to rect_corners in math.hpp
	resources::transition_tile_type get_transition_type(const std::array<bool, 4u>&) noexcept;
	//Iter1 and Iter 2 must point to terrain*
	template<typename Iter1, typename Iter2>
	resources::transition_tile_type get_transition_type(tile_corners, Iter1 begin, Iter2 end) noexcept;

	tile_corners get_terrain_at_tile(const terrain_map&, tile_position, const resources::terrain_settings&);

	struct triangle_height_data
	{
		// contains height for both triangles
		// See terrain_map for more details
		std::array<std::uint8_t, 6> height;
		terrain_map::triangle_type triangle_type;
	};

	// returns height arranged into two triangles
	triangle_height_data get_height_for_triangles(tile_position, const terrain_map&);

	std::array<std::uint8_t, 4> get_max_height_in_corners(const tile_position tile_index, const terrain_map& map);
	std::array<std::uint8_t, 4> get_max_height_in_corners(const triangle_height_data& tris) noexcept;

	// returns the height at both end of an edge (left/top first)
	constexpr std::array<std::uint8_t, 2> get_height_for_top_edge(const triangle_height_data&) noexcept;
	constexpr std::array<std::uint8_t, 2> get_height_for_left_edge(const triangle_height_data&) noexcept;
	constexpr std::array<std::uint8_t, 2> get_height_for_right_edge(const triangle_height_data&) noexcept;
	constexpr std::array<std::uint8_t, 2> get_height_for_bottom_edge(const triangle_height_data&) noexcept;
	// as above, but left triangle first and right triangle second
	constexpr std::array<std::uint8_t, 4> get_height_for_diag_edge(const triangle_height_data& tris) noexcept;

	// NOTE: cliffs are *owned* by the tile they are attached too
	//		eg: a tile owns cliffs that pass through its middle, and
	//			cliffs to it's right and bottom
	// This info is used mostly for rendering
	terrain_map::cliff_info get_cliff_info(tile_index_t, const terrain_map&) noexcept;
	terrain_map::cliff_info get_cliff_info(tile_position, const terrain_map&);


	// index array using rectangle_math.hpp::hades::rect_corners
	// useful for deciding the surface tile near cliffs
	std::array<bool, 4> get_cliffs_corners(tile_position, const terrain_map&);
	
	const resources::terrain *get_vertex(const terrain_map&, terrain_vertex_position);

	//NOTE: the result of this function may contain duplicates
	// TODO: require settings to be passed to this function
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

	// calls Func on each tile that borders the passed vertex
	template<std::invocable<tile_position> Func>
	void for_each_adjacent_tile(terrain_vertex_position, const terrain_map&, Func&& f) noexcept;
	template<std::invocable<tile_position> Func>
	void for_each_safe_adjacent_tile(terrain_vertex_position, const terrain_map&, Func&& f) noexcept;

	// Same as for_each_adjacent_tile except Func will be passed the corner that represents
	//	the origional vertex as an argument
	template<std::invocable<tile_position, rect_corners> Func>
	static void for_each_safe_adjacent_corner(const terrain_map& m, const terrain_vertex_position p, Func&& f)
		noexcept(std::is_nothrow_invocable_v<Func, tile_position, rect_corners>);

	// place terrain on a vertex (tiles will be generated automatically)
	void place_terrain(terrain_map&, terrain_vertex_position, const resources::terrain*, const resources::terrain_settings&);
	// place a specific terrain gfx on a tile, will also mark the adjacent vertex as this terrain
	// TODO: implement
	void place_terrain(terrain_map&, tile_position, const resources::tile&, const resources::terrain*, const resources::terrain_settings&);
	
	// update the height of a single terrain vertex
	template<invocable_r<std::uint8_t, std::uint8_t> Func>
	void change_terrain_height(terrain_vertex_position vert, terrain_map&, const resources::terrain_settings&, Func&&);

	// TODO: change_cliff_layer

	// project 'p' onto the flat version of 'map'
	world_vector_t project_onto_terrain(world_vector_t p, float rot_degrees,
		resources::tile_size_t tile_size, const terrain_map& map);
}

#include "hades/detail/terrain.inl"

#endif // !HADES_TERRAIN_HPP
