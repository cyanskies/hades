#include "hades/curve_types.hpp"

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
	case curve_variable_type::collection_int:
		return "collection-int32"s;
	case curve_variable_type::collection_float:
		return "collection-float"s;
	case curve_variable_type::collection_object_ref:
		return "collection-obj-ref"s;
	case curve_variable_type::collection_unique:
		return "collection-unique"s;
	case curve_variable_type::error:
		[[fallthrough]];
	default:
		return "error"s;
	}
}

namespace hades::curve_types
{
	template<typename T, std::enable_if_t<is_curve_type_v<T>, int>>
	constexpr curve_variable_type type_to_curve_type() noexcept
	{
		if constexpr (std::is_same_v<T, int_t>)
			return curve_variable_type::int_t;
		else if constexpr (std::is_same_v<T, float_t>)
			return curve_variable_type::float_t;
		else if constexpr (std::is_same_v<T, vec2_float>)
			return curve_variable_type::vec2_float;
		else if constexpr (std::is_same_v<T, bool_t>)
			return curve_variable_type::bool_t;
		else if constexpr (std::is_same_v<T, string>)
			return curve_variable_type::string;
		else if constexpr (std::is_same_v<T, object_ref>)
			return curve_variable_type::collection_object_ref;
		else if constexpr (std::is_same_v<T, unique>)
			return curve_variable_type::unique;
		else if constexpr (std::is_same_v<T, colour>)
			return curve_variable_type::colour;
		else if constexpr (std::is_same_v<T, collection_int>)
			return curve_variable_type::collection_int;
		else if constexpr (std::is_same_v<T, collection_float>)
			return curve_variable_type::collection_float;
		else if constexpr (std::is_same_v<T, collection_object_ref>)
			return curve_variable_type::collection_object_ref;
		else if constexpr (std::is_same_v<T, collection_unique>)
			return curve_variable_type::collection_unique;
		else
			return curve_variable_type::error;
	}

	template<typename T, std::enable_if_t<is_curve_type_v<T>, int>>
	string curve_type_to_string() noexcept
	{
		return to_string(type_to_curve_type<T>());
	}
}
