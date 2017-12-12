#ifndef ORTHO_TERRAIN_RESOURCES_HPP
#define ORTHO_TERRAIN_RESOURCES_HPP

#include "Hades/resource_base.hpp"

#include "OrthoTerrain/utility.hpp"
#include "Tiles/resources.hpp"

//tiles namespace is for the terrain editing and rendering subsystem
namespace ortho_terrain
{
	void RegisterOrthoTerrainResources(hades::data::data_manager*);
	//tile sizes are capped by the data type used for texture sizes
	using tile_size_t = tiles::tile_size_t;

	//terrain info, mapped to tiles give information about terrain and transitions
	struct terrain_info
	{
		hades::data::UniqueId terrain = hades::data::UniqueId::Zero,
			terrain2 = hades::data::UniqueId::Zero,
			terrain3 = hades::data::UniqueId::Zero,
			terrain4 = hades::data::UniqueId::Zero;

		union
		{
			transition2::TransitionTypes type;
			//only used if terrain3 is non-Zero
			transition3::Types type3;
		};
	};

	//terrain map
	using terrain_lookup = std::map<tiles::tile, terrain_info>;
	extern terrain_lookup TerrainLookup;

	namespace resources
	{	
		const hades::types::string terrain_settings_name = "terrain-settings";

		struct terrain_settings_t {};

		struct terrain_settings : public hades::resources::resource_type<terrain_settings_t>
		{
			hades::data::UniqueId error_terrain = hades::data::UniqueId::Zero;
		};

		struct terrain_transition_t {};

		//a list of tiles used for terrain transitions for a particular pair of terrain
		struct terrain_transition : public hades::resources::resource_type<terrain_transition_t>
		{
			hades::data::UniqueId terrain1 = hades::data::UniqueId::Zero, terrain2 = hades::data::UniqueId::Zero;

			//the terrain transition tiles
			//named as terrain1 interacts with terrain 2
			std::vector<tiles::tile>
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
		};

		struct terrain_transition3_t {};

		struct terrain_transition3 : public hades::resources::resource_type<terrain_transition3_t>
		{
			hades::data::UniqueId terrain1 = hades::data::UniqueId::Zero, terrain2 = hades::data::UniqueId::Zero,
				terrain3 = hades::data::UniqueId::Zero;

			//the 2-corner transitions that are used to complete the set
			hades::data::UniqueId transition_1_2 = hades::data::UniqueId::Zero,
				transition_2_3 = hades::data::UniqueId::Zero, transition_1_3 = hades::data::UniqueId::Zero;
			
			//the terrain transition tiles
			//only contains tiles that cover all three terrains
			//named to identify which corners are not terrain1
			//use GetTransition to access these
			std::vector<tiles::tile>
				t2_bottom_right_t3_top_right, t2_top_right_t3_bottom_right, t2_bottom_left_t3_top_right, t2_bottom_left_right_t3_top_right,
				t2_bottom_left_t3_bottom_right, t2_top_right_bottom_left_t3_bottom_right, t2_bottom_left_t3_top_right_bottom_right, t2_top_right_t3_bottom_left,
				t2_bottom_right_t3_bottom_left, t2_top_right_bottom_right_t3_bottom_left, t2_bottom_right_t3_top_right_bottom_left, t2_top_right_t3_bottom_left_right,
				/////////////////////
				t2_top_left_t3_top_right, t2_top_left_bottom_right_t3_top_right, t2_top_left_t3_bottom_right, t2_top_left_right_t3_bottom_right,
				t2_top_left_t3_top_right_bottom_right, t2_top_left_bottom_left_t3_top_right, t2_top_left_bottom_left_t3_bottom_right, t2_top_left_t3_bottom_left,
				t2_top_left_right_t3_bottom_left, t2_top_left_t3_top_right_bottom_left, t2_top_left_bottom_right_t3_bottom_left, t2_top_left_t3_bottom_left_right,
				/////////////////////
				t2_top_right_t3_top_left, t2_bottom_right_t3_top_left, t2_top_right_bottom_right_t3_top_left, t2_bottom_right_t3_top_left_right,
				t2_top_right_t3_top_left_bottom_right, t2_bottom_left_t3_top_left, t2_top_right_bottom_left_t3_top_left, t2_bottom_left_t3_top_left_right,
				t2_bottom_left_right_t3_top_left, t2_bottom_left_t3_top_left_bottom_right, t2_top_right_t3_top_left_bottom_left, t2_bottom_right_t3_top_left_bottom_left;	
		};

		struct terrain_t {};

		//a terrain types, with random tiles for that terrain and transitions
		struct terrain : public hades::resources::resource_type<terrain_t>
		{
			//a terrain contains all the references to tiles needed to render that terrain
			std::vector<tiles::tile> tiles; //list of random tiles for this terrain

			//list of terrain-transition objects, linking this terrain to others
			std::vector<hades::data::UniqueId> transitions;
			std::vector<hades::data::UniqueId> transitions3;

			tiles::traits_list traits;
		};

		//global list of terrains for the editor to show in the ui
		//editor can access individual tiles and transition tiles from in here as well
		extern std::vector<hades::data::UniqueId> Terrains;
	}
}

#endif // !ORTHO_TERRAIN_RESOURCES_HPP
