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
	void RegisterObjectResources(hades::data::data_manager*);

	namespace resources
	{
		struct object
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
			//systems
			//client system
		};
	}
}

#endif // !OBJECTS_RESOURCES_HPP
