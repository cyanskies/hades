#ifndef HADES_OBJECTS_HPP
#define HADES_OBJECTS_HPP

#include <vector>

#include "hades/data.hpp"
#include "hades/game_system.hpp"
#include "hades/resource_base.hpp"
#include "hades/types.hpp"
#include "hades/writer.hpp"

namespace hades::resources
{
	struct animation;
	
	struct object_t
	{};

	struct object : public resource_type<object_t>
	{
		object();

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


	extern std::vector<const object*> all_objects;
}

namespace hades
{
	void register_objects(hades::data::data_manager&);

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

	object_instance make_instance(const resources::object&);

	//functions for getting info from objects
	//checks base classes if the requested info isn't available in the current class
	//NOTE: performs a depth first search for the requested data
	using curve_list = resources::object::curve_list;
	using curve_value = hades::resources::curve_default_value;
	//NOTE: the following curve functions throw curve_not_found if the object doesn't have that curve
	// or the value hasn't been set
	curve_value get_curve(const object_instance &o, const hades::resources::curve &c);
	curve_value get_curve(const resources::object &o, const hades::resources::curve &c);
	void set_curve(object_instance &o, const hades::resources::curve &c, curve_value v);
	curve_list get_all_curves(const object_instance &o); // < collates all unique curves from the class tree
	curve_list get_all_curves(const resources::object &o); // < prefers data from decendants over ancestors
	//ensures that a vector curve(position/size/etc...) have a valid state(2 elements)
	//NOTE: usually used as const auto value = ValidVectorCurve(GetCurve(object, curve_ptr));
	curve_value valid_vector_curve(hades::resources::curve_default_value v);
	string get_object_name(const resources::object &o);
	const hades::resources::animation *get_editor_icon(const resources::object &o);
	resources::object::animation_list get_editor_animations(const resources::object &o);

	struct level;
	//reads the object tree from the level and writes it
	void write_objects_from_level(const level&, data::writer&);
	//read objects from the parser and save them in the level
	void read_objects_into_level(const data::parser_node&, level&);
}

#endif // !OBJECTS_OBJECTS_HPP
