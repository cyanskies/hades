#ifndef HADES_CURVE_TYPES_HPP
#define HADES_CURVE_TYPES_HPP

#include <tuple>

#include "hades/colour.hpp"
#include "hades/entity_id.hpp"
#include "hades/string.hpp"
#include "hades/time.hpp"
#include "hades/types.hpp"
#include "hades/uniqueid.hpp"
#include "hades/vector_math.hpp"

namespace hades
{
	struct game_obj; //fwd
} 

namespace hades
{
	enum class curve_variable_type {
		error, int_t, /*int64_t, int64 was used to hold times before time_d was fixed */ float_t, vec2_float, bool_t,
		string, object_ref, unique, colour, time_d, collection_int, collection_float,
		collection_object_ref, collection_unique, collection_colour, collection_time_d
	};

	string to_string(curve_variable_type) noexcept;

	// held by objects to refer to one another
	struct object_ref
	{
		using id_type = entity_id;
		entity_id id = bad_entity;
		game_obj* ptr = nullptr;
	};

	constexpr bool operator<(const object_ref& l, const object_ref& r) noexcept
	{
		return l.id < r.id;
	}

	constexpr bool operator==(const object_ref& l, const object_ref& r) noexcept
	{
		return l.id == r.id;
	}

	constexpr bool operator!=(const object_ref& l, const object_ref& r) noexcept
	{
		return l.id != r.id;
	}

	template<>
	inline object_ref from_string<object_ref>(std::string_view s)
	{
		return { from_string<entity_id>(s) };
	}

	inline string to_string(object_ref o)
	{
		return to_string(o.id);
	}
}

namespace hades::curve_types
{
	using int_t = int32;
	//using int64_t = int64;
	using float_t = float;
	using vec2_float = vector_float;
	//TODO: double_t
	using bool_t = bool;
	using string = hades::string;
	using object_ref = hades::object_ref;
	using unique = unique_id;
	using colour = hades::colour;
	using time_d = hades::time_duration;
	using collection_int = std::vector<int_t>;
	using collection_float = std::vector<float_t>;
	using collection_object_ref = std::vector<object_ref>;
	using collection_unique = std::vector<unique>;
	using collection_colour = std::vector<colour>;
	using collection_time_d = std::vector<time_d>;

	using type_pack = std::tuple<
		int_t,
		//int64_t,
		float_t,
		vec2_float,
		bool_t,
		string,
		object_ref,
		unique,
		colour,
		time_d,
		collection_int,
		collection_float,
		collection_object_ref,
		collection_unique,
		collection_colour,
		collection_time_d
	>;

	constexpr auto bad_object_ref = object_ref{};
	constexpr auto bad_unique = unique_zero;

	// is_curve_type::value is true for any type in the type_pack
	template<typename T, typename = void>
	struct is_curve_type : std::false_type {};
	template<typename T>
	struct is_curve_type<T,
		std::void_t<decltype(std::get<T>(std::declval<type_pack>()))>
	> : std::true_type {};

	template<typename T>
	constexpr auto is_curve_type_v = is_curve_type<T>::value;

	// test if a type is a mathematical vector {x, y}
	template <typename T>
	struct is_vector_type : std::false_type {};
	template <typename T>
	struct is_vector_type<vector_t<T>> : is_curve_type<vector_t<T>> {};
	template <typename T>
	constexpr auto is_vector_type_v = is_vector_type<T>::value;

	namespace detail
	{
		template<typename T>
		struct is_std_vector : std::false_type {};
		template<typename T>
		struct is_std_vector<std::vector<T>> : std::true_type {};
	}

	// test if a type is a collection, std::vector<T>
	template <typename T>
	struct is_collection_type :
		std::bool_constant<is_curve_type_v<T> && detail::is_std_vector<T>::value> {};
	template<typename T>
	constexpr auto is_collection_type_v = is_collection_type<T>::value;

	//true if we can perform a linear interpolation for this type
	template<typename T>
	constexpr auto is_linear_interpable_v =
		std::is_same_v<T, int_t> ||
		std::is_same_v<T, float_t> ||
		std::is_same_v<T, vec2_float> ||
		std::is_same_v<T, colour> ||
		std::is_same_v<T, time_d>;

	template<typename T, std::enable_if_t<is_curve_type_v<T>, int> = 0>
	constexpr curve_variable_type type_to_curve_type() noexcept;

	template<typename T, std::enable_if_t<is_curve_type_v<T>, int> = 0>
	string curve_type_to_string() noexcept;
}

#include "detail/curve_types.inl"

#endif //HADES_CURVE_TYPES_HPP
