#include "hades/core_curves.hpp"
#include "hades/data.hpp"

namespace hades
{
	static auto name_id = unique_id::zero;
	static auto posx_id = unique_id::zero;
	static auto posy_id = unique_id::zero;
	static auto sizx_id = unique_id::zero;
	static auto sizy_id = unique_id::zero;
	static auto collision_groups_id = unique_id::zero;
	static auto tags_id = unique_id::zero;
	static auto object_type_id = unique_id::zero;

	static void setup_curve(resources::curve &c)
	{
		c.c_type = curve_type::linear;
		c.data_type = resources::curve_variable_type::float_t;
		c.default_value = 0.f;
	}

	void register_core_curves(data::data_manager &d)
	{
		using namespace std::string_view_literals;
		using resources::curve;
		// the objects name, usually the name of the object type
		name_id = d.get_uid("name"sv);
		auto name_c = d.find_or_create<curve>(name_id, unique_id::zero);
		name_c->c_type = curve_type::step;
		name_c->data_type = resources::curve_variable_type::string;
		name_c->default_value = string{};

		// the position curves store the objects 2d position
		posx_id = d.get_uid("position-x"sv);
		setup_curve(*d.find_or_create<curve>(posx_id, unique_id::zero));
		posy_id = d.get_uid("position-y"sv);
		setup_curve(*d.find_or_create<curve>(posy_id, unique_id::zero));
		// size curves store the objects 2d size
		sizx_id = d.get_uid("size-x"sv);
		setup_curve(*d.find_or_create<curve>(sizx_id, unique_id::zero));
		sizy_id = d.get_uid("size-y"sv);
		setup_curve(*d.find_or_create<curve>(sizy_id, unique_id::zero));
		// collision groups are used to check whether two objects collide
		// or to check what terrain an object can move on
		//TODO: should a sperate collision-terrain list be used for terrain collision?
		collision_groups_id = d.get_uid("collision-groups"sv);
		auto col_groups = d.find_or_create<curve>(collision_groups_id, unique_id::zero);

		col_groups->c_type = curve_type::step;
		col_groups->data_type = resources::curve_variable_type::vector_unique;
		col_groups->default_value = resources::curve_types::vector_unique{};

		// tags for this object
		tags_id = d.get_uid("tags"sv);
		auto tags = d.find_or_create<curve>(tags_id, unique_id::zero);
		tags->c_type = curve_type::step;
		tags->data_type = resources::curve_variable_type::vector_unique;
		tags->default_value = resources::curve_types::vector_unique{};
		// object type is the unique_id of the objects type
		object_type_id = d.get_uid("object-type"sv);
		auto obj_type = d.find_or_create<curve>(object_type_id, unique_id::zero);
		obj_type->c_type = curve_type::const_c;
		obj_type->data_type = resources::curve_variable_type::unique;
		obj_type->default_value = resources::curve_types::unique::zero;
	}

	static const resources::curve *get_curve(unique_id i)
	{
		// if !i then register core curves was never called
		assert(i);
		return data::get<resources::curve>(i);
	}

	const resources::curve* get_name_curve()
	{
		return get_curve(name_id);
	}

	vector_curve get_position_curve()
	{
		return { get_curve(posx_id), get_curve(posy_id) };
	}
	
	vector_curve get_size_curve()
	{
		return { get_curve(sizx_id), get_curve(sizy_id) };
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
}