#ifndef OBJECTS_RESOURCES_HPP
#define OBJECTS_RESOURCES_HPP

#include <tuple>
#include <vector>

#include "Hades/resource_base.hpp"
#include "Hades/simple_resources.hpp"

//objects are a resource type representing spawnable entities
//everything from doodads to players

namespace objects
{
	//console variable names for editor settings
	//editor setting variables
	const auto editor_scroll_margin = "editor_scroll_area",//int
		editor_scroll_rate = "editor_scroll_rate", //int
		//Grid settings
		editor_snaptogrid = "editor_snaptogrid", //int(see SnapToGrid
		editor_grid_size = "editor_grid_min_size", //int
		editor_grid_max = "editor_grid_max", //int
		editor_grid_enabled = "editor_show_grid",  //bool
		editor_grid_size_multiple = "editor_grid_size", //int
		//map settings
		editor_map_size = "editor_map_size", //int
		//object settings
		editor_object_mock_size = "editor_object_mock_size"; //int

	enum SnapToGrid { GRIDSNAP_FORCE_DISABLED = -1, GRIDSNAP_DISABLED, GRIDSNAP_ENABLED,  GRIDSNAP_FORCE_ENABLED };

	const auto editor_scroll_margin_default = 8;
	const auto editor_scroll_rate_default = 4;
	const auto editor_snap_default = GRIDSNAP_ENABLED;
	const auto editor_grid_default = 8;
	const auto editor_map_size_default = 100;
	const auto editor_mock_size_default = editor_grid_default;

	void RegisterObjectResources(hades::data::data_manager*);
	
	namespace resources
	{
		const auto editor_settings_name = "editor";

		struct editor_t 
		{};

		struct editor : public hades::resources::resource_type<editor_t>
		{
			bool show_grid_settings = true,
				show_grid_snap = true;
			//grid colour
			//grid highlight colour
			sf::Color grid = sf::Color::Black, grid_highlight = sf::Color::White;
			//toolbar icons
			hades::resources::animation *selection_mode_icon = nullptr,
				*grid_show_icon = nullptr,
				*grid_shrink_icon = nullptr,
				*grid_grow_icon = nullptr,
				*grid_snap_icon = nullptr;

			//object mock colour
			sf::Color object_mock_colour = sf::Color::Cyan;
		};

		struct object_t
		{};

		struct object : public hades::resources::resource_type<object_t>
		{
			//editor icon, used in the object picker
			const hades::resources::animation *editor_icon = nullptr;
			//editor anim list
			//used for placed objects
			using animation_list = std::vector<const hades::resources::animation*>;
			animation_list editor_anims;
			//base objects, curves and systems are inherited from these
			using base_list = std::vector<const object*>;
			base_list base;
			//list of curves
			using curve_obj = std::tuple<const hades::resources::curve*, hades::resources::curve_default_value>;
			using curve_list = std::vector<curve_obj>;
			curve_list curves;
			//TODO:
			//systems
			//client system
		};

		struct object_group
		{
			hades::types::string name = "OBJECT_GROUP_NAME";
			std::vector<const object*> obj_list;
		};

		extern std::vector<const object*> Objects;
		extern std::vector<object_group> ObjectGroups;
	}
}

#endif // !OBJECTS_RESOURCES_HPP
