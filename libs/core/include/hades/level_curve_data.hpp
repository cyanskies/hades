#ifndef HADES_LEVEL_CURVE_DATA_HPP
#define HADES_LEVEL_CURVE_DATA_HPP

#include "hades/curve_extra.hpp"
#include "hades/shared_map.hpp"
#include "hades/game_system.hpp"
#include "hades/types.hpp"

namespace hades
{
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
	inline curve_data::curve_map<T>& get_curve_list(curve_data& data)
	{
		assert(false);
		throw std::logic_error{ "invalid type" };
	}

	template<>
	curve_data::curve_map<resources::curve_types::int_t>& get_curve_list(curve_data& data);

	template<>
	curve_data::curve_map<resources::curve_types::float_t>& get_curve_list(curve_data& data);

	template<>
	curve_data::curve_map<resources::curve_types::bool_t>& get_curve_list(curve_data& data);

	template<>
	curve_data::curve_map<resources::curve_types::string>& get_curve_list(curve_data& data);

	template<>
	curve_data::curve_map<resources::curve_types::object_ref>& get_curve_list(curve_data& data);

	template<>
	curve_data::curve_map<resources::curve_types::unique>& get_curve_list(curve_data& data);

	template<>
	curve_data::curve_map<resources::curve_types::vector_int>& get_curve_list(curve_data& data);

	template<>
	curve_data::curve_map<resources::curve_types::vector_float>& get_curve_list(curve_data& data);

	template<>
	curve_data::curve_map<resources::curve_types::vector_object_ref>& get_curve_list(curve_data& data);

	template<>
	curve_data::curve_map<resources::curve_types::vector_unique>& get_curve_list(curve_data& data);

}

#endif //!HADES_LEVEL_CURVE_DATA_HPP
