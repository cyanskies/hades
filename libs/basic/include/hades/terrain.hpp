#ifndef HADES_TERRAIN_HPP
#define HADES_TERRAIN_HPP

#include "hades/resource_base.hpp"

#include "hades/data.hpp"
#include "hades/tiles.hpp"

//tiles namespace is for the terrain editing and rendering subsystem
namespace hades
{
	void register_terrain_resources(data::data_manager&);
}

namespace hades::resources
{
	struct terrain : tileset
	{
		terrain();

		//named as this terrain interacts with empty
		//NOTE: WARNING: This is the reverse of the intuitive naming scheme
		std::vector<tile>
			full, // whole tile is terrain
			top_left_corner,	//only the bottom right is empty
			top_right_corner,	//only the bottom left is empty
			bottom_left_corner, //only the top right is empty
			bottom_right_corner,//only the top left is empty
			top,	//bottom is empty
			left,	//right is empty
			right,	//left is empty
			bottom, //top is empty
			top_left_circle,	// all is empty except bottom right
			top_right_circle,	// all is empty except bottom left
			bottom_left_circle, // all is empty except top right
			bottom_right_circle,// all is empty except top left
			left_diagonal, //empty in top right, and bottom left corners
			right_diagonal; // empty in top left, and bottom right corners

		//NOTE: also has access to std::vector<tile> tiles
		// which contains all the tiles from the above lists as well

		tag_list tags;
	};

	struct terrainset_t {};

	struct terrainset : resource_type<terrainset_t>
	{
		terrainset();

		std::vector<const terrain*> terrains;
	};

	struct terrain_settings_t {};

	struct terrain_settings : tile_settings
	{
		terrain_settings();

		const terrain *empty_terrain = nullptr;
		std::vector<const terrain*> terrains;
		std::vector<const terrainset*> terrainsets;
	};
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
