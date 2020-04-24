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
		return "obj_ref"s;
	case curve_variable_type::unique:
		return "unique"s;
	case curve_variable_type::collection_int:
		return "collection_int32"s;
	case curve_variable_type::collection_float:
		return "collection_float"s;
	case curve_variable_type::collection_object_ref:
		return "collection_obj_ref"s;
	case curve_variable_type::collection_unique:
		return "collection_unique"s;
	case curve_variable_type::error:
		[[fallthrough]];
	default:
		return "error"s;
	}
}

namespace hades::curve_types
{
	template<typename T, std::enable_if_t<is_curve_type_v<T>, int>>
	string curve_type_to_string() noexcept
	{
		using namespace std::string_literals;
		if constexpr (std::is_same_v<T, int_t>)
			return "int32"s;
		else if constexpr (std::is_same_v<T, float_t>)
			return "float"s;
		else if constexpr (std::is_same_v<T, vec2_float>)
			return "vec2_float"s;
		else if constexpr (std::is_same_v<T, bool_t>)
			return "bool"s;
		else if constexpr (std::is_same_v<T, string>)
			return "string"s;
		else if constexpr (std::is_same_v<T, object_ref>)
			return "obj_ref"s;
		else if constexpr (std::is_same_v<T, unique>)
			return "unique"s;
		else if constexpr (std::is_same_v<T, collection_int>)
			return "collection_int32"s;
		else if constexpr (std::is_same_v<T, collection_float>)
			return "collection_float"s;
		else if constexpr (std::is_same_v<T, collection_object_ref>)
			return "collection_obj_ref"s;
		else if constexpr (std::is_same_v<T, collection_unique>)
			return "collection_unique"s;
	}
}
