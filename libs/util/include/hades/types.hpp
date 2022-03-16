#ifndef HADES_UTIL_TYPES_HPP
#define HADES_UTIL_TYPES_HPP

#include <cmath>
#include <cstdint>
#include <limits>
#include <tuple>
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
		template<typename ...Types> struct always_false : public std::false_type {};

		template<typename ...Ts>
		constexpr auto always_false_v = always_false<Ts...>::value;

		template<typename ...Ts>
        constexpr auto always_true_v = !always_false_v<Ts...>;
	}

	using namespace types;

	//engines support types

	//limit is 65535, well in excess of current hardware capabilities
	//  max texture size for older hardware is 512
	//  max size for modern hardware is 8192 or higher
	using texture_size_t = uint16;

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

	// detect iterable
	template<typename, typename = void, typename = void>
	struct is_iterable : std::false_type { };
	template<typename T>
	struct is_iterable<T,
		std::void_t<decltype(std::declval<T>().begin())>,
		std::void_t<decltype(std::declval<T>().end())>>
		: std::true_type { };

	template<typename T>
	constexpr auto is_iterable_v = is_iterable<T>::value;
}

#endif //HADES_UTIL_TYPES_HPP
