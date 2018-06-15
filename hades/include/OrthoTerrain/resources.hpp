#ifndef ORTHO_TERRAIN_RESOURCES_HPP
#define ORTHO_TERRAIN_RESOURCES_HPP

#include "Hades/resource_base.hpp"

#include "OrthoTerrain/utility.hpp"
#include "Tiles/resources.hpp"

//tiles namespace is for the terrain editing and rendering subsystem
namespace ortho_terrain
{
	void CreateEmptyTerrain(hades::data::data_system*);
	void RegisterOrthoTerrainResources(hades::data::data_system*);
	//tile sizes are capped by the data type used for texture sizes
	using tiles::tile_size_t;
	
	namespace resources
	{	
		struct terrain_t {};

		//a terrain types, with random tiles for that terrain and transitions
		struct terrain : public tiles::resources::tileset
		{
			terrain();

			//named as this terrain interacts with empty
			//NOTE: WARNING: This is the reverse of the intuitive naming scheme
			std::vector<tiles::tile>
				full, // whole tile is terrain1
				top_left_corner,	//only the bottom right is terrain2
				top_right_corner,	//only the bottom left is terrain2
				bottom_left_corner, //only the top right is terrain2
				bottom_right_corner,//only the top left is terrain2
				top,	//bottom is terrain2
				left,	//right is terrain2
				right,	//left is terrain2
				bottom, //top is terrain2
				top_left_circle,	// all is terrain2 except bottom right
				top_right_circle,	// all is terrain2 except bottom left
				bottom_left_circle, // all is terrain2 except top right
				bottom_right_circle,// all is terrain2 except top left
				left_diagonal, //terrain2 in top right, and bottom left corners
				right_diagonal; // terrain2 in top left, and bottom right corners

			//traits are additive and are applied to all the tile lists contained in a terrain
			tiles::traits_list traits;
		};

		struct terrainset_t {};

		struct terrainset : public hades::resources::resource_type<terrainset_t>
		{
			std::vector<const terrain*> terrains;
		};

		//global list of terrainsets for the editor to show in the ui
		//editor can access individual tiles and transition tiles from in here as well
		extern std::vector<hades::unique_id> TerrainSets;
		extern hades::unique_id EmptyTerrainId;
	}
}

#endif // !ORTHO_TERRAIN_RESOURCES_HPP
