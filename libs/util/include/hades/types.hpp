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
	template<typename T>
	concept is_tuple = requires
	{
		std::tuple_size<T>::value;
	};

	template<typename T>
	concept Tuple = requires
	{
		std::tuple_size<std::remove_reference<T>>::value;
	};
}

#endif //HADES_UTIL_TYPES_HPP
