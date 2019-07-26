#ifndef HADES_LEVEL_CURVE_DATA_HPP
#define HADES_LEVEL_CURVE_DATA_HPP

#include "hades/curve_extra.hpp"
#include "hades/shared_map.hpp"
#include "hades/types.hpp"

namespace hades
{
	//this isn't needed for EntityId's and Entity names are strings, and rarely used, where
	//curves need to be identified often by a consistant lookup name
	//we do the same with variable Ids since they also need to be unique and easily network transferrable
	using variable_id = unique_id;
	const variable_id bad_variable = variable_id::zero;

	using name_curve_t = curve<std::map<types::string, entity_id>>;
	using curve_index_t = std::pair<entity_id, variable_id>;

	struct curve_data
	{
		template<class T>
		using curve_map = shared_map< curve_index_t, curve<T> >;

		curve_map<resources::curve_types::int_t> int_curves;
		curve_map<resources::curve_types::float_t> float_curves;
		//no linear curves here
		curve_map<resources::curve_types::bool_t> bool_curves;
		curve_map<resources::curve_types::string> string_curves;
		curve_map<resources::curve_types::object_ref> object_ref_curves;
		curve_map<resources::curve_types::unique> unique_curves;

		//no linear curves here either
		curve_map<resources::curve_types::vector_int> int_vector_curves;
		curve_map<resources::curve_types::vector_float> float_vector_curves;
		curve_map<resources::curve_types::vector_object_ref> object_ref_vector_curves;
		curve_map<resources::curve_types::vector_unique> unique_vector_curves;
	};

	template<typename T>
	inline curve_data::curve_map<std::decay_t<T>>& get_curve_list(curve_data& data) 
		noexcept(resources::curve_types::is_curve_type_v<std::decay_t<T>>);
}

#include "hades/detail/level_curve_data.inl"

#endif //!HADES_LEVEL_CURVE_DATA_HPP
