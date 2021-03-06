#ifndef HADES_OBJECTS_HPP
#define HADES_OBJECTS_HPP

#include <vector>

#include "hades/curve_extra.hpp"
#include "hades/data.hpp"
#include "Hades/game_types.hpp"
#include "hades/resource_base.hpp"
#include "hades/vector_math.hpp"
#include "hades/writer.hpp"

namespace hades::resources
{
	struct animation;
	struct curve;
	struct render_system;
	struct system;
	
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

	//TODO: hide behind func
	extern std::vector<const object*> all_objects;
}

namespace hades
{
	//registers animations and core curves as well
	void register_objects(hades::data::data_manager&);

	//represents an instance of an object in a level file
	struct object_instance
	{
		const resources::object *obj_type = nullptr;
		entity_id id = bad_entity;
		//NOTE: name_id is used to address objects using a name rather than id
		// for the objects display name, shown to users, look up the name curve
		string name_id;
		time_point creation_time;
		resources::object::curve_list curves;
	};

	class curve_not_found : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	object_instance make_instance(const resources::object*);

	//functions for getting info from objects
	//checks base classes if the requested info isn't available in the current class
	//NOTE: performs a depth first search for the requested data
	using curve_list = resources::object::curve_list;
	using curve_value = hades::resources::curve_default_value;

	bool has_curve(const object_instance &o, const resources::curve &c);
	bool has_curve(const resources::object &o, const resources::curve &c);
	//NOTE: the following curve functions throw curve_not_found if the object doesn't have that curve
	// or the value hasn't been set
	curve_value get_curve(const object_instance &o, const hades::resources::curve &c);
	curve_value get_curve(const resources::object &o, const hades::resources::curve &c);
	void set_curve(object_instance &o, const hades::resources::curve &c, curve_value v);

	void set_curve(object_instance& o, const unique_id i, curve_value v); 
	void set_curve(resources::object& o, const hades::resources::curve& c, curve_value v);
	void set_curve(resources::object& o, const unique_id i, curve_value v);

	curve_list get_all_curves(const object_instance &o); // < collates all unique curves from the class tree
	curve_list get_all_curves(const resources::object &o); // < prefers data from decendants over ancestors

	std::vector<const resources::system*> get_systems(const resources::object& o);
	std::vector<const resources::render_system*> get_render_systems(const resources::object& o);

	const hades::resources::animation *get_editor_icon(const resources::object &o);
	const hades::resources::animation *get_random_animation(const object_instance &o);
	resources::object::animation_list get_editor_animations(const resources::object &o);
	resources::object::animation_list get_editor_animations(const object_instance &o);

	//helpers for common curves
	//NOTE: can throw curve_error(if the curve resource doesn't exist
	// and curve_not_found(if the object doesn't have that particular curve)
	string get_name(const object_instance &o);
	string get_name(const resources::object &o);
	void set_name(object_instance &o, std::string_view);
	resources::curve_types::vec2_float get_position(const object_instance &o);
	resources::curve_types::vec2_float get_position(const resources::object &o);
	void set_position(object_instance &o, resources::curve_types::vec2_float v);
	resources::curve_types::vec2_float get_size(const object_instance &o);
	resources::curve_types::vec2_float get_size(const resources::object &o);
	void set_size(object_instance &o, resources::curve_types::vec2_float v);

	tag_list get_collision_groups(const object_instance &o);
	tag_list get_collision_groups(const resources::object &o);

	tag_list get_tags(const object_instance &o);
	tag_list get_tags(const resources::object &o);

	//container for saved object data
	struct object_data
	{
		std::vector<object_instance> objects;
		//the id of the next entity to be placed, or spawned in-game
		entity_id next_id = next(bad_entity);
	};

	void serialise(const object_data&, data::writer&);
	object_data deserialise_object_data(data::parser_node&);
}

#endif // !OBJECTS_OBJECTS_HPP
