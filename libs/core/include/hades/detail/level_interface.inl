#include "hades/level_interface.hpp"

namespace hades
{
	template<typename T>
	curve_data::curve_map<T> *get_curve_list(curve_data &data)
	{
		assert(false);
		throw std::logic_error{"invalid type"};
	}

	template<>
	curve_data::curve_map<resources::curve_types::int_t> &get_curve_list(curve_data &data)
	{
		return data.int_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::float_t> &get_curve_list(curve_data &data)
	{
		return data.float_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::bool_t> &get_curve_list(curve_data &data)
	{
		return data.bool_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::string> &get_curve_list(curve_data &data)
	{
		return data.string_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::object_ref> &get_curve_list(curve_data &data)
	{
		return data.object_ref_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::unique> &get_curve_list(curve_data &data)
	{
		return data.unique_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::vector_int> &get_curve_list(curve_data &data)
	{
		return data.vector_int_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::vector_float> &get_curve_list(curve_data &data)
	{
		return data.vector_float_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::vector_object_ref> &get_curve_list(curve_data &data)
	{
		return data.vector_object_ref_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::vector_unique> &get_curve_list(curve_data &data)
	{
		return data.vector_unique_curves;
	}
}