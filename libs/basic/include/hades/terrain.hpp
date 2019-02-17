#ifndef HADES_TERRAIN_HPP
#define HADES_TERRAIN_HPP

#include "hades/resource_base.hpp"

#include "hades/data.hpp"
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
	//named based on which tile corners are 'empty'
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
		std::array<std::vector<tile>, top_left_bottom_left_right> terrain_transition_tiles;

		//NOTE: also has access to std::vector<tile> tiles
		// which contains all the tiles from the above lists as well

		tag_list tags;
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
		std::vector<const terrain*> terrains;
		std::vector<const terrainset*> terrainsets;
	};

	std::vector<tile> &get_transitions(terrain&, transition_tile_type);
	const std::vector<tile> &get_transitions(const terrain&, transition_tile_type);
}

namespace hades
{
	struct raw_terrain_map
	{
		raw_map tile_layer;
	};

	struct terrain_map
	{
		//a terrainset lists the terrain types that can be used in a level
		const resources::terrainset *terrainset;

		//vertex of terrain
		std::vector<const resources::terrain*> terrain_vertex;
		std::vector<tile_map> terrain_layers;
	
		tile_map tile_layer;
	};

	//type for positioning vertex in a terrain map
	using terrain_vertex_position = tile_position;

	terrain_vertex_position get_size(const terrain_map&);

	//index tile_corners using rect_corners from math.hpp
	using tile_corners = std::array<const resources::terrain*, 4u>;
	const resources::terrain *get_corner(const tile_corners&, rect_corners);


	tile_corners get_terrain_at_tile(const terrain_map&, tile_position);
	//get terrain at vertex
	//get terrainquad from tile
}

#endif // !HADES_TERRAIN_HPP
