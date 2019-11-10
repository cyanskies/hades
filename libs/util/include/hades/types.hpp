#ifndef HADES_UTIL_TYPES_HPP
#define HADES_UTIL_TYPES_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <type_traits>

namespace hades {
	namespace types {
		//portable types for integers
		using int8 = std::int8_t;
		using uint8 = std::uint8_t;

		using int16 = std::int16_t;
		using uint16 = std::uint16_t;

		using int32 = std::int32_t;
		using uint32 = std::uint32_t;

		using int64 = std::int64_t;
		using uint64 = std::uint64_t;

		template<typename Float, typename = std::enable_if_t<std::is_floating_point_v<Float> && std::numeric_limits<Float>::has_infinity>>
		constexpr auto infinity = std::numeric_limits<Float>::infinity();

		//defined names for common types
		using string = std::string;

		template<typename ...Types> struct always_false : public std::false_type {};

		template<typename ...Ts>
		constexpr auto always_false_v = always_false<Ts...>::value;

		template<typename ...Ts>
        constexpr auto always_true_v = !always_false_v<Ts...>;
	}

	using namespace types;

	// identify string types, these can all be converted to hades::string
	template<typename T> struct is_string : std::false_type{};
	template<> struct is_string<string> : std::true_type {};
	template<> struct is_string<const char*> : std::true_type {};
	template<> struct is_string<std::string_view> : std::true_type {};

	template<typename T>
	constexpr bool is_string_v = is_string<T>::value;

	//identify tuple-like types
	//these should support tuple_size, tuple_element and get<>
	template<typename T, typename = void>
	struct is_tuple : std::false_type {};

	//black magic for checking for a complete definition of tuple_size<T>
	template<typename T>
	struct is_tuple<T,
		std::enable_if_t<sizeof(std::tuple_size<T>) == sizeof(std::tuple_size<T>)>> 
		: std::true_type {};

	template<typename T>
	constexpr auto is_tuple_v = is_tuple<T>::value;
}

#endif //HADES_UTIL_TYPES_HPP
