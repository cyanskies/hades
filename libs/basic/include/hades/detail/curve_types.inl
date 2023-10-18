#include "hades/curve_types.hpp"

#include <utility>
#include <type_traits>

inline hades::string hades::to_string(hades::curve_variable_type t) noexcept
{
	using namespace std::string_literals;
	switch (t)
	{
	case curve_variable_type::int_t:
		return "int32"s;
	case curve_variable_type::float_t:
		return "float"s;
	case curve_variable_type::vec2_float:
		return "vec2_float"s;
	case curve_variable_type::bool_t:
		return "bool"s;
	case curve_variable_type::string:
		return "string"s;
	case curve_variable_type::object_ref:
		return "obj-ref"s;
	case curve_variable_type::unique:
		return "unique"s;
	case curve_variable_type::colour:
		return "colour"s;
	case curve_variable_type::time_d:
		return "time-d"s;
	case curve_variable_type::collection_int:
		return "collection-int32"s;
	case curve_variable_type::collection_float:
		return "collection-float"s;
	case curve_variable_type::collection_object_ref:
		return "collection-obj-ref"s;
	case curve_variable_type::collection_unique:
		return "collection-unique"s;
	/*case curve_variable_type::collection_colour:
		return "collection-colour"s;*/
	case curve_variable_type::collection_time_d:
		return "collection-time-d"s;
	case curve_variable_type::error:
		;
	}

	return "error"s;
}

constexpr hades::curve_variable_type hades::curve_collection_element_type(curve_variable_type t) noexcept
{
	using ty = curve_variable_type;
	switch (t)
	{
	case ty::collection_int:
		return ty::int_t;
	case ty::collection_float:
		return ty::float_t;
	case ty::collection_object_ref:
		return ty::object_ref;
	case ty::collection_unique:
		return ty::unique;
	/*case ty::collection_colour:
		return ty::colour;*/
	case ty::collection_time_d:
		return ty::time_d;
	}

	return t;
}

template <>
struct std::hash<hades::object_ref>
{
	size_t operator()(const hades::object_ref& key) const noexcept
	{
		using id_type = hades::object_ref::id_type;
		const auto h = std::hash<id_type::value_type>{};
		return h(static_cast<id_type::value_type>(key.id));
	}
};
