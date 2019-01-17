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
		name_id = d.get_uid("name"sv);
		auto name_c = d.find_or_create<curve>(name_id, unique_id::zero);
		name_c->c_type = curve_type::step;
		name_c->data_type = resources::curve_variable_type::string;
		name_c->default_value = string{};

		posx_id = d.get_uid("position-x"sv);
		setup_curve(*d.find_or_create<curve>(posx_id, unique_id::zero));
		posy_id = d.get_uid("position-y"sv);
		setup_curve(*d.find_or_create<curve>(posy_id, unique_id::zero));
		sizx_id = d.get_uid("size-x"sv);
		setup_curve(*d.find_or_create<curve>(sizx_id, unique_id::zero));
		sizy_id = d.get_uid("size-y"sv);
		setup_curve(*d.find_or_create<curve>(sizy_id, unique_id::zero));
		collision_groups_id = d.get_uid("collision-groups");
		auto *col_groups = d.find_or_create<curve>(collision_groups_id, unique_id::zero);

		col_groups->c_type = curve_type::step;
		col_groups->data_type = resources::curve_variable_type::vector_unique;
		col_groups->default_value = resources::curve_types::vector_unique{};

		tags_id = d.get_uid("tags");
		auto *tags = d.find_or_create<curve>(tags_id, unique_id::zero);
		tags->c_type = curve_type::step;
		tags->data_type = resources::curve_variable_type::vector_unique;
		tags->default_value = resources::curve_types::vector_unique{};
	}

	static const resources::curve *get_curve(unique_id i)
	{
		if (!i)
			throw curve_error{"requested curve was not registered to the data manager: " + data::get_as_string(i)};

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
}