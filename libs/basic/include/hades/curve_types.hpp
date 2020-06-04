#ifndef HADES_CURVE_TYPES_HPP
#define HADES_CURVE_TYPES_HPP

#include "hades/colour.hpp"
#include "hades/entity_id.hpp"
#include "hades/types.hpp"
#include "hades/uniqueid.hpp"
#include "hades/vector_math.hpp"

namespace hades
{
	enum class curve_variable_type {
		error, int_t, float_t, vec2_float, bool_t,
		string, object_ref, unique, colour, collection_int, collection_float,
		collection_object_ref, collection_unique
	};

	string to_string(curve_variable_type) noexcept;
}

namespace hades::curve_types
{
	using int_t = int32;
	using float_t = float;
	using vec2_float = vector_float;
	//TODO: double_t
	using bool_t = bool;
	using string = types::string;
	using object_ref = entity_id;
	using unique = unique_id;
	using colour = hades::colour;
	using collection_int = std::vector<int_t>;
	using collection_float = std::vector<float_t>;
	using collection_object_ref = std::vector<object_ref>;
	using collection_unique = std::vector<unique>;

	constexpr auto bad_object_ref = bad_entity;
	constexpr auto bad_unique = unique_zero;

	template <typename T>
	struct is_vector_type : std::false_type {};
	template <>
	struct is_vector_type<vec2_float> : public std::true_type {};

	template <typename T>
	constexpr auto is_vector_type_v = is_vector_type<T>::value;

	template <typename T>
	struct is_collection_type : public std::false_type {};
	template <>
	struct is_collection_type<collection_int> : public std::true_type {};
	template <>
	struct is_collection_type<collection_float> : public std::true_type {};
	template <>
	struct is_collection_type<collection_object_ref> : public std::true_type {};
	template <>
	struct is_collection_type<collection_unique> : public std::true_type {};

	template<typename T>
	constexpr auto is_collection_type_v = is_collection_type<T>::value;

	template<typename T>
	struct is_curve_type : public std::false_type {};

	template<>
	struct is_curve_type<int_t> : public std::true_type {};
	template<>
	struct is_curve_type<float_t> : public std::true_type {};
	template<>
	struct is_curve_type<vec2_float> : public std::true_type {};
	template<>
	struct is_curve_type<bool_t> : public std::true_type {};
	template<>
	struct is_curve_type<string> : public std::true_type {};
	template<>
	struct is_curve_type<object_ref> : public std::true_type {};
	template<>
	struct is_curve_type<unique> : public std::true_type {};
	template<>
	struct is_curve_type<colour> : public std::true_type {};
	template<>
	struct is_curve_type<collection_int> : public std::true_type {};
	template<>
	struct is_curve_type<collection_float> : public std::true_type {};
	template<>
	struct is_curve_type<collection_object_ref> : public std::true_type {};
	template<>
	struct is_curve_type<collection_unique> : public std::true_type {};

	template<typename T>
	constexpr auto is_curve_type_v = is_curve_type<T>::value;

	template<typename T, std::enable_if_t<is_curve_type_v<T>, int> = 0>
	constexpr curve_variable_type type_to_curve_type() noexcept;

	template<typename T, std::enable_if_t<is_curve_type_v<T>, int> = 0>
	string curve_type_to_string() noexcept;
}

#include "detail/curve_types.inl"

#endif //HADES_CURVE_TYPES_HPP
