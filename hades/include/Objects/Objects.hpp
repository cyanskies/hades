#ifndef OBJECTS_OBJECTS_HPP
#define OBJECTS_OBJECTS_HPP

#include <vector>

#include "Hades/GameSystem.hpp"
#include "Hades/simple_resources.hpp"
#include "Hades/Types.hpp"

#include "Objects/resources.hpp"

namespace objects
{
	//represents an instance of an object in a level file
	struct object_info
	{
		const resources::object *obj_type;
		hades::EntityId id;
		resources::object::curve_list curves;
	};

	//functions for getting info from objects
	//checks base classes if the requested info isn't available in the current class
	//NOTE: performs a depth first search for the requested data
	using curve_obj = resources::object::curve_obj;
	using curve_list = resources::object::curve_list;
	curve_obj GetCurve(const object_info &o, hades::data::UniqueId c);
	curve_obj GetCurve(const resources::object *o, hades::data::UniqueId c);
	curve_list GetAllCurves(const object_info &o); // < collates all unique curves from the class tree
	curve_list GetAllCurves(const resources::object *o); // < prefers data from decendants over ancestors
	const hades::resources::animation *GetEditorIcon(const resources::object *o);
	resources::object::animation_list GetEditorAnimations(const resources::object *o);

	using level_size_t = hades::types::uint32;

	//A level is laid out in the same way as a save file
	// but doesn't store the full curve history
	struct level
	{
		//map size in pixels
		level_size_t map_x = 0, map_y = 0;
		//sequence of object
		    //list of curve values, not including defaults
		std::vector<object_info> objects;
		//the id of the next entity to be placed, or spawned in-game
		hades::EntityId next_id = hades::NO_ENTITY;

		//TODO: background, flat colour, paralax image, paralax loop image
		//set paralax to 0 to get a static image
		//layered images with different paralax?
	};
}

#endif // !OBJECTS_OBJECTS_HPP
