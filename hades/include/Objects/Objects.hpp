#ifndef OBJECTS_OBJECTS_HPP
#define OBJECTS_OBJECTS_HPP

#include <vector>

#include "Hades/GameSystem.hpp"
#include "Hades/simple_resources.hpp"
#include "Hades/Types.hpp"

#include "Objects/resources.hpp"

namespace YAML
{
	class Node;
	class Emitter;
}

namespace objects
{
	//represents an instance of an object in a level file
	struct object_info
	{
		const resources::object *obj_type = nullptr;
		hades::EntityId id = hades::NO_ENTITY;
		hades::types::string name;
		resources::object::curve_list curves;
	};

	class curve_not_found : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	//functions for getting info from objects
	//checks base classes if the requested info isn't available in the current class
	//NOTE: performs a depth first search for the requested data
	using curve_list = resources::object::curve_list;
	using curve_value = hades::resources::curve_default_value;
	//NOTE: the following curve functions throw curve_not_found if the object doesn't have that curve
	// or the value hasn't been set
	curve_value GetCurve(const object_info &o, const hades::resources::curve *c);
	curve_value GetCurve(const resources::object *o, const hades::resources::curve *c);
	void SetCurve(object_info &o, const hades::resources::curve *c, curve_value v);
	curve_list GetAllCurves(const object_info &o); // < collates all unique curves from the class tree
	curve_list GetAllCurves(const resources::object *o); // < prefers data from decendants over ancestors
	//ensures that a vector curve(position/size/etc...) have a valid state(2 elements)
	//NOTE: usually used as const auto value = ValidVectorCurve(GetCurve(object, curve_ptr));
	curve_value ValidVectorCurve(hades::resources::curve_default_value v);
	const hades::resources::animation *GetEditorIcon(const resources::object *o);
	resources::object::animation_list GetEditorAnimations(const resources::object *o);

	using level_size_t = hades::types::uint32;

	constexpr auto level_ext = "lvl";
	constexpr auto save_ext = "hsv";
	//A level is laid out in the same way as a save file
	// but doesn't store the full curve history
	struct level
	{
		hades::types::string name;
		hades::types::string description;

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

	//reads object and basic map data from the yaml node and stores it in target
	//includes map size, map description/title, background and so on
	//also includes all the object related data, next ID, all objects
	void ReadObjectsFromYaml(const YAML::Node&, level &target);
	//does the reverse of the above function
	YAML::Emitter &WriteObjectsToYaml(const level&, YAML::Emitter &);
}

#endif // !OBJECTS_OBJECTS_HPP
