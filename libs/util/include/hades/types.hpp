#ifndef HADES_UTIL_TYPES_HPP
#define HADES_UTIL_TYPES_HPP

#include <algorithm>
#include <cstdint>
#include <string>
#include <type_traits>

namespace hades {
	namespace types {
		//portable types for integers
		using int8 = int_fast8_t;
		using uint8 = uint_fast8_t;

		using int16 = int_fast16_t;
		using uint16 = uint_fast16_t;

		using int32 = int_fast32_t;
		using uint32 = uint_fast32_t;

		using int64 = int_fast64_t;
		using uint64 = uint_fast64_t;

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
		constexpr auto always_true_v = !always_false_v<Ts...>::value;

		//string to value conversion
		template<typename T>
		T stov(std::string value)
		{
            //TODO: gcc is too eager to interpret static asserts, need a different solution here
			static_assert(always_false<T>::value, "Tried to convert type not covered by stov");
			return T();
		}
	}

	using namespace types;

	template<typename T> struct is_string : std::false_type{};
	template<> struct is_string<string> : std::true_type {};
	template<> struct is_string<std::string_view> : std::true_type {};

	template<typename T>
	constexpr auto is_string_v = is_string<T>::value;
}

#endif //HADES_UTIL_TYPES_HPP
