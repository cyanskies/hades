#ifndef HADES_CURVE_TYPES_HPP
#define HADES_CURVE_TYPES_HPP

//TODO: rename to game data types

#include <tuple>

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
		collection_object_ref, collection_unique, collection_colour
	};

	string to_string(curve_variable_type) noexcept;

	struct game_obj;
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

	template<>
	inline string to_string<object_ref>(object_ref o)
	{
		return to_string(o.id);
	}
}

namespace hades::curve_types
{
	using int_t = int32;
	using float_t = float;
	using vec2_float = vector_float;
	//TODO: double_t
	using bool_t = bool;
	using string = types::string;
	using object_ref = hades::object_ref;
	using unique = unique_id;
	using colour = hades::colour;
	using collection_int = std::vector<int_t>;
	using collection_float = std::vector<float_t>;
	using collection_object_ref = std::vector<object_ref>;
	using collection_unique = std::vector<unique>;
	using collection_colour = std::vector<colour>;

	using type_pack = std::tuple<
		int_t,
		float_t,
		vec2_float,
		bool_t,
		string,
		object_ref,
		unique,
		colour,
		collection_int,
		collection_float,
		collection_object_ref,
		collection_unique,
		collection_colour
	>;

	constexpr auto bad_object_ref = object_ref{};
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
	template <>
	struct is_collection_type<collection_colour> : public std::true_type {};

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
	template<>
	struct is_curve_type<collection_colour> : public std::true_type {};

	template<typename T>
	constexpr auto is_curve_type_v = is_curve_type<T>::value;

	template<typename T, std::enable_if_t<is_curve_type_v<T>, int> = 0>
	constexpr curve_variable_type type_to_curve_type() noexcept;

	template<typename T, std::enable_if_t<is_curve_type_v<T>, int> = 0>
	string curve_type_to_string() noexcept;
}

#include "detail/curve_types.inl"

#endif //HADES_CURVE_TYPES_HPP
