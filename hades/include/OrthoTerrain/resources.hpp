#ifndef ORTHO_TERRAIN_RESOURCES_HPP
#define ORTHO_TERRAIN_RESOURCES_HPP

#include "Hades/resource_base.hpp"

#include "OrthoTerrain/utility.hpp"
#include "Tiles/resources.hpp"

//tiles namespace is for the terrain editing and rendering subsystem
namespace ortho_terrain
{
	void RegisterOrthoTerrainResources(hades::data::data_system*);
	//tile sizes are capped by the data type used for texture sizes
	using tile_size_t = tiles::tile_size_t;

	//terrain info, mapped to tiles give information about terrain and transitions
	struct terrain_info
	{
		hades::data::UniqueId terrain = hades::data::UniqueId::Zero;
		transition2::TransitionTypes type;
	};

	constexpr auto terrain_settings_name = "terrain-settings";
	namespace resources
	{	
		tiles::TileArray& GetMutableErrorTileset(hades::data::data_manager*);

		struct terrain_t {};

		//a terrain types, with random tiles for that terrain and transitions
		struct terrain : public hades::resources::resource_type<terrain_t>
		{
			//named as this terrain interacts with empty
			std::vector<tiles::tile>
				tile, // whole tile is terrain1
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

			tiles::traits_list traits;
		};

		struct terrain_settings_t {};

		struct terrain_settings : public hades::resources::resource_type<terrain_settings_t>
		{
			const terrain *empty_terrrainset = nullptr;
		};

		struct terrainset_t {};

		struct terrainset : public hades::resources::resource_type<terrainset_t>
		{
			std::vector<terrain*> terrains;
		};

		//global list of terrains for the editor to show in the ui
		//editor can access individual tiles and transition tiles from in here as well
		extern std::vector<hades::data::UniqueId> TerrainSets;
	}
}

#endif // !ORTHO_TERRAIN_RESOURCES_HPP
