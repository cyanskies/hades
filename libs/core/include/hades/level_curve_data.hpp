#ifndef HADES_LEVEL_CURVE_DATA_HPP
#define HADES_LEVEL_CURVE_DATA_HPP

#include <unordered_map>

#include "hades/curve.hpp"
#include "hades/curve_types.hpp"
#include "hades/types.hpp"
#include "hades/uniqueid.hpp"

namespace hades
{
	//this isn't needed for EntityId's and Entity names are strings, and rarely used, where
	//curves need to be identified often by a consistant lookup name
	//we do the same with variable Ids since they also need to be unique and easily network transferrable
	using variable_id = unique_id;
	const variable_id bad_variable = variable_id::zero;

	using name_curve_t = std::map<types::string, entity_id>;
	using curve_index_t = std::pair<entity_id, variable_id>;
}

namespace std {
	template<>
	struct hash<hades::curve_index_t> {
	public:
		size_t operator()(const hades::curve_index_t& c) const
		{
			// NOTE: algorithm sourced from boost::hash_combine_impl:310
			// https://github.com/boostorg/container_hash/blob/develop/include/boost/container_hash/hash.hpp
			size_t h1 = std::hash<hades::entity_id::value_type>{}(hades::to_value(c.first));
			size_t h2 = std::hash<hades::variable_id::type>{}(c.second.get());
			return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
		}
	};
}

namespace hades
{
	struct curve_data
	{
		template<class T>
		using curve_map = std::unordered_map< curve_index_t, game_property_curve<T> >;

		curve_map<curve_types::int_t> int_curves;
		curve_map<curve_types::float_t> float_curves;
		curve_map<curve_types::vec2_float> vec2_float_curves;
		//no linear curves here
		curve_map<curve_types::bool_t> bool_curves;
		curve_map<curve_types::string> string_curves;
		curve_map<curve_types::object_ref> object_ref_curves;
		curve_map<curve_types::unique> unique_curves;

		//no linear curves here either
		curve_map<curve_types::collection_int> int_vector_curves;
		curve_map<curve_types::collection_float> float_vector_curves;
		curve_map<curve_types::collection_object_ref> object_ref_vector_curves;
		curve_map<curve_types::collection_unique> unique_vector_curves;
	};

	template<typename T>
	inline curve_data::curve_map<std::decay_t<T>>& get_curve_list(curve_data& data) 
		noexcept(curve_types::is_curve_type_v<std::decay_t<T>>);

	template<typename T>
	inline const curve_data::curve_map<std::decay_t<T>>& get_curve_list(const curve_data& data)
		noexcept(curve_types::is_curve_type_v<std::decay_t<T>>);
}

#include "hades/detail/level_curve_data.inl"

#endif //!HADES_LEVEL_CURVE_DATA_HPP
