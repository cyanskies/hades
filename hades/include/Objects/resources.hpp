#ifndef OBJECTS_RESOURCES_HPP
#define OBJECTS_RESOURCES_HPP

#include <vector>

#include "Hades/resource_base.hpp"

//objects are a resource type representing spawnable entities
//everything from doodads to players

namespace objects
{
	void RegisterObjectResources(hades::data::data_manager*);

	namespace resources
	{
		struct object
		{
			//editor icon
			//editor animation: (editor animation id), [anim1, anim2, anim3]
			//base: object base, [object base 1, object base 2]
			//curves
			//systems
			//client system
		};
	}
}

#endif // !OBJECTS_RESOURCES_HPP
