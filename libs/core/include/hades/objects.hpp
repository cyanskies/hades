#ifndef HADES_OBJECTS_HPP
#define HADES_OBJECTS_HPP

#include <vector>

#include "hades/curve_extra.hpp"
#include "hades/data.hpp"
#include "hades/exceptions.hpp"
#include "hades/game_types.hpp"
#include "hades/resource_base.hpp"
#include "hades/vector_math.hpp"
#include "hades/writer.hpp"

namespace hades
{
	class object_error : public runtime_error
	{
	public :
		using runtime_error::runtime_error;
	};

	class curve_not_found : public object_error
	{
	public:
		using object_error::object_error;
	};

	class curve_already_present : public object_error
	{
	public:
		using object_error::object_error;
	};
}

namespace hades::resources
{
	struct animation;
	struct animation_group;
	struct curve;
	struct render_system;
	struct system;

	struct object_t
	{};

	struct object : public resource_type<object_t>
	{
		void load(data::data_manager&) final override;
		void serialise(const data::data_manager&, data::writer&) const final override;
		//editor icon, used in the object picker
		resource_link<resources::animation_group> animations;
		// [[deprecated("use animation groups instead")]]
		//editor anim list
		//used for placed objects
		using animation_list = std::vector<resource_link<animation>>;
		animation_list editor_anims; // TODO: can we fit this in animation_group too somehow
		//base objects, curves and systems are inherited from these
		using base_list = std::vector<resource_link<object>>;
		base_list base;
		//list of curves
		// NOTE: curves need  to be defined before other resources can reference them
		//		no benifit to using resource_link here
		struct curve_obj
		{
			curve_default_value value;
            const curve* curve_ptr = {};
			unique_id object;
		};

		using curve_list = std::vector<curve_obj>;

		using unloaded_value = std::variant<std::monostate, string, std::vector<string>>;
		struct unloaded_curve
		{
            resource_link<curve> curve_link;
			unloaded_value value;
		};

		using unloaded_curve_list = std::vector<unloaded_curve>;
		unloaded_curve_list curves;	// just the curves added by this object when parsed
		curve_list all_curves; // all the curves on this object: calculated on load()

		//server systems
		using system_list = std::vector<resource_link<system>>;
		system_list systems,
			all_systems;

		//client systems
		using render_system_list = std::vector<resource_link<render_system>>;
		render_system_list render_systems,
			all_render_systems;

		// tags
		tag_list tags,
			all_tags;
	};

	//TODO: hide behind func, deprecate and use resource manager instead
	extern std::vector<resource_link<object>> all_objects;
}

namespace hades::resources::object_functions
{
	unique_id get_id(const object& o) noexcept;
	const object::curve_list& get_all_curves(const object& o) noexcept;
	const std::vector<resource_link<system>>& get_systems(const object& o) noexcept;
	const std::vector<resource_link<render_system>>& get_render_systems(const object& o) noexcept;
	const tag_list& get_tags(const object& o) noexcept;
	struct inherited_tag_entry
	{
		unique_id tag, object;
	};
	using inherited_tag_list = std::vector<inherited_tag_entry>;
	inherited_tag_list get_inherited_tags(const object&);

	// utility func for converting curve values back to the unloaded string format
	object::unloaded_value to_unloaded_curve(const curve& c, curve_default_value v);

	// modifying curves
	// add curve to object, overriding inherited values
	// or update the default value for this curve.
	void add_curve(object&, unique_id, std::optional<curve_default_value> = {});
	bool has_curve(const object& o, const curve& c) noexcept;
	bool has_curve(const object& o, unique_id) noexcept;
	//NOTE: the following curve functions throw curve_not_found if the object doesn't have that curve
	// remove curve will just reset the curve to its inherited value if one of its bases still has the curve
	// exceptions: curve_not_found
	void remove_curve(object&, unique_id);
	
	// exceptions: curve_not_found
	curve_default_value get_curve(const object& o, const curve& c);
	// exceptions: curve_not_found
	const object::curve_obj& get_curve(const object& o, unique_id);
}

namespace hades
{
	//registers animations and core curves as well
	void register_objects(hades::data::data_manager&);

	//represents an instance of an object in a level file
	//also used to prefab objects before creating them
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

	// instance of an object in a save file
	struct object_save_instance
	{
		const resources::object* obj_type = nullptr;
		entity_id id = bad_entity;
		string name_id = {};
		time_point creation_time{};
		time_point destruction_time = time_point::max();

		struct saved_curve
		{
			struct saved_keyframe
			{
				time_point time{};
				resources::curve_default_value value{};
			};

			const resources::curve* curve;
			std::vector<saved_keyframe> keyframes;
		};

		std::vector<saved_curve> curves;
	};

	object_instance make_instance(const resources::object*) noexcept;
	object_save_instance make_save_instance(object_instance obj_instance);

	//functions for getting info from objects
	//checks base classes if the requested info isn't available in the current class
	//NOTE: performs a depth first search for the requested data
	using curve_list = resources::object::curve_list;
	using curve_value = hades::resources::curve_default_value;

	bool has_curve(const object_instance &o, const resources::curve &c) noexcept;
	bool has_curve(const object_instance& o, unique_id) noexcept;
	//NOTE: the following curve functions throw curve_not_found if the object doesn't have that curve
	// or the value hasn't been set
	curve_value get_curve(const object_instance &o, const hades::resources::curve &c);

	// TODO: objects.inl
	template<curve_type CurveType>
	CurveType get_curve_value(const object_instance& o, const resources::curve& curve)
	{
		const auto c = get_curve(o, curve);
		assert(std::holds_alternative<CurveType>(c));
		return std::get<CurveType>(c);
	}

	void set_curve(object_instance &o, const hades::resources::curve &c, curve_value v);
	void set_curve(object_instance& o, const unique_id i, curve_value v); 

	curve_list get_all_curves(const object_instance &o); // < collates all unique curves from the class tree
	
	const resources::animation *get_editor_icon(const resources::object &o);
	const resources::animation *get_random_animation(const object_instance &o);
	std::vector<const resources::animation*> get_editor_animations(const resources::object &o);
	std::vector<const resources::animation*> get_editor_animations(const object_instance &o);
	const resources::animation* get_animation(const resources::object& o, unique_id);

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

	unique_id get_collision_groups(const object_instance &o);
	unique_id get_collision_groups(const resources::object &o);

	//container for object data in level fiels
	struct object_data
	{
		std::vector<object_instance> objects;
		//the id of the next entity to be placed, or spawned in-game
		entity_id next_id = next(bad_entity);
	};

	//container for saved object data
	struct object_save_data
	{
		std::vector<object_save_instance> objects;
		entity_id next_id = bad_entity;
	};

	void serialise(const object_data&, data::writer&);
	object_data deserialise_object_data(data::parser_node&);
}

#endif // !OBJECTS_OBJECTS_HPP
