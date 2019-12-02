#include "hades/core_curves.hpp"
#include "hades/data.hpp"

namespace hades
{
	static auto alive_id = unique_id::zero;
	static auto name_id = unique_id::zero;
	static auto pos_id = unique_id::zero;
	static auto siz_id = unique_id::zero;
	static auto collision_groups_id = unique_id::zero;
	static auto tags_id = unique_id::zero;
	static auto object_type_id = unique_id::zero;

	static void setup_position_curve(data::data_manager& d, unique_id id)
	{
		resources::make_curve(d, id, curve_type::linear,
			resources::curve_variable_type::vec2_float,
			resources::curve_types::vec2_float{ 0.f, 0.f },
			true, true);
		return;
	}

	void register_core_curves(data::data_manager &d)
	{
		register_curve_resource(d);

		using namespace std::string_view_literals;
		using resources::curve;

		//TODO: use make_curve to produce these, so that they are properly registered

		alive_id = d.get_uid("alive"sv);
		resources::make_curve(d, alive_id, curve_type::step, 
			resources::curve_variable_type::bool_t, true, true, true);

		// the objects name, usually the name of the object type
		name_id = d.get_uid("name"sv);
		resources::make_curve(d, name_id, curve_type::step, resources::curve_variable_type::string, string{}, true, true);

		// the position curves store the objects 2d position
		pos_id = d.get_uid("position"sv);
		setup_position_curve(d, pos_id);
		// size curves store the objects 2d size
		siz_id = d.get_uid("size"sv);
		setup_position_curve(d, siz_id);
		
		// collision groups are used to check whether two objects collide
		// or to check what terrain an object can move on
		//TODO: should a sperate collision-terrain list be used for terrain collision?
		collision_groups_id = d.get_uid("collision-groups"sv);
		resources::make_curve(d, collision_groups_id, curve_type::step,
			resources::curve_variable_type::collection_unique,
			resources::curve_types::collection_unique{},
			false, //sync to client
			false); //save to file

		// tags for this object
		tags_id = d.get_uid("tags"sv);
		resources::make_curve(d, tags_id, curve_type::step,
			resources::curve_variable_type::collection_unique,
			resources::curve_types::collection_unique{},
			true, // sync to client
			false); // save to file

		// object type is the unique_id of the objects type
		object_type_id = d.get_uid("object-type"sv);
		resources::make_curve(d, object_type_id, curve_type::const_c,
			resources::curve_variable_type::unique,
			resources::curve_types::unique::zero,
			true, //sync to client
			false); // save to file

		return;
	}

	static const resources::curve *get_curve(unique_id i)
	{
		// if !i then register core curves was never called
		assert(i);
		return data::get<resources::curve>(i);
	}

	//TODO: should these all get the values as static?
	const resources::curve* get_name_curve()
	{
		return get_curve(name_id);
	}

	const resources::curve* get_position_curve()
	{
		return get_curve(pos_id);
	}
	
	const resources::curve* get_size_curve()
	{
		return get_curve(siz_id);
	}

	const resources::curve * get_collision_group_curve()
	{
		return get_curve(collision_groups_id);
	}

	const resources::curve *get_tags_curve()
	{
		return get_curve(tags_id);
	}

	const resources::curve* get_object_type_curve()
	{
		return get_curve(object_type_id);
	}

	unique_id get_alive_curve_id() noexcept
	{
		return alive_id;
	}

	unique_id get_position_curve_id() noexcept
	{
		return pos_id;
	}

	unique_id get_size_curve_id() noexcept
	{
		return siz_id;
	}

	unique_id get_object_type_curve_id() noexcept
	{
		return object_type_id;
	}
}