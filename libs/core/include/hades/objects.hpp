#ifndef HADES_OBJECTS_HPP
#define HADES_OBJECTS_HPP

#include <vector>

//needs systems, which needs GameInterface
//which needs RenderInterface, which needs ExportedCurves
//which needs...

//needs animations	//! animations depends on textures and shaders
					//  it's not being moved into basic, the anim 
					//functions here will have to become part of the editor

//editor icon and editor anim in resources::object will have
//to be stored as unique_id or something.

//needs level.hpp
//needs entityId type

#include "hades/data.hpp"
#include "hades/types.hpp"
#include "hades/game_system.hpp"
#include "hades/resource_base.hpp"

namespace hades::resources
{
	struct animation;

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

		//server systems
		using system_list = std::vector<const hades::resources::system*>;
		system_list systems;

		//client systems
		using render_system_list = std::vector<const hades::resources::render_system*>;
		render_system_list render_systems;
	};
}

namespace hades
{
	void register_objects(hades::data::data_manager*);

	//represents an instance of an object in a level file
	struct object_instance
	{
		const resources::object *obj_type = nullptr;
		entity_id id = bad_entity;
		string name;
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
	curve_value GetCurve(const object_instance &o, const hades::resources::curve *c);
	curve_value GetCurve(const resources::object *o, const hades::resources::curve *c);
	void SetCurve(object_instance &o, const hades::resources::curve *c, curve_value v);
	curve_list GetAllCurves(const object_instance &o); // < collates all unique curves from the class tree
	curve_list GetAllCurves(const resources::object *o); // < prefers data from decendants over ancestors
	//ensures that a vector curve(position/size/etc...) have a valid state(2 elements)
	//NOTE: usually used as const auto value = ValidVectorCurve(GetCurve(object, curve_ptr));
	curve_value ValidVectorCurve(hades::resources::curve_default_value v);
	const hades::resources::animation *GetEditorIcon(const resources::object *o);
	resources::object::animation_list GetEditorAnimations(const resources::object *o);

	//reads object and basic map data from the yaml node and stores it in target
	//includes map size, map description/title, background and so on
	//also includes all the object related data, next ID, all objects
	//void ReadObjectsFromYaml(const YAML::Node&, hades::level &target);
	//does the reverse of the above function
	//YAML::Emitter &WriteObjectsToYaml(const hades::level&, YAML::Emitter &);
}

#endif // !OBJECTS_OBJECTS_HPP
