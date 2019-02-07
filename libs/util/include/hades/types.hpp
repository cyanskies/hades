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
		using int8 = std::int_fast8_t;
		using uint8 = std::uint_fast8_t;

		using int16 = std::int_fast16_t;
		using uint16 = std::uint_fast16_t;

		using int32 = std::int_fast32_t;
		using uint32 = std::uint_fast32_t;

		using int64 = std::int_fast64_t;
		using uint64 = std::uint_fast64_t;

		//portable types for float;
		using float32 = std::float_t;
		using float64 = std::double_t;

		template<typename Float, typename = std::enable_if_t<std::is_floating_point_v<Float> && std::numeric_limits<Float>::has_infinity>>
		constexpr auto infinity = std::numeric_limits<Float>::infinity();

		//defined names for common types
		using string = std::string;

		//using bool
		//using float
		//using double
		//using long double

		template<typename ...Types> struct always_false : public std::false_type {};

		template<typename ...Ts>
		constexpr auto always_false_v = always_false<Ts...>::value;

		template<typename ...Ts>
        constexpr auto always_true_v = !always_false_v<Ts...>;
	}

	using namespace types;

	template<typename T> struct is_string : std::false_type{};
	template<> struct is_string<string> : std::true_type {};
	template<> struct is_string<const char*> : std::true_type {};
	template<> struct is_string<std::string_view> : std::true_type {};

	template<typename T>
	constexpr bool is_string_v = is_string<T>::value;
}

#endif //HADES_UTIL_TYPES_HPP
