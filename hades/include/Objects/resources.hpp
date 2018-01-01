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
	const auto editor_snaptogrid = "editor_snaptogrid";

	void RegisterObjectResources(hades::data::data_manager*);
	
	namespace resources
	{
		const auto editor_settings_name = "editor";

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
