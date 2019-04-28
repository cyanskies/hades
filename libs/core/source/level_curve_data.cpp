#include "hades/level_curve_data.hpp"

namespace hades
{
	template<>
	curve_data::curve_map<resources::curve_types::int_t>& get_curve_list(curve_data& data)
	{
		return data.int_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::float_t>& get_curve_list(curve_data& data)
	{
		return data.float_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::bool_t>& get_curve_list(curve_data& data)
	{
		return data.bool_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::string>& get_curve_list(curve_data& data)
	{
		return data.string_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::object_ref>& get_curve_list(curve_data& data)
	{
		return data.object_ref_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::unique>& get_curve_list(curve_data& data)
	{
		return data.unique_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::vector_int>& get_curve_list(curve_data& data)
	{
		return data.int_vector_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::vector_float>& get_curve_list(curve_data& data)
	{
		return data.float_vector_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::vector_object_ref>& get_curve_list(curve_data& data)
	{
		return data.object_ref_vector_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::vector_unique>& get_curve_list(curve_data& data)
	{
		return data.unique_vector_curves;
	}
}