#ifndef HADES_TYPES_HPP
#define HADES_TYPES_HPP

#include <cstdint>
#include <string>
#include <type_traits>

#include "SFML/Config.hpp"

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
		template<typename T>
		constexpr bool hades_type()
		{
			//return true for the above types
			return std::is_arithmetic<T>::value || std::is_same<T, string>::value;
		}

		//string to value conversion
		template<typename T>
		T stov(std::string value)
		{
            //TODO: gcc is too eager to interpret static asserts, need a different solution here
			//static_assert(false, "Tried to convert type not covered by stov");
			return T();
		}

		// type conversion
		// TODO: remove all but the specialisations needed by console
		// and curves
		template<>
		signed char stov<signed char>(std::string value);
		template<>
		unsigned char stov<unsigned char>(std::string value);
		template<>
		short stov<short>(std::string value);
		template<>
		unsigned short stov<unsigned short>(std::string value);
		template<>
		int stov<int>(std::string value);
		template<>
		unsigned int stov<unsigned int>(std::string value);
		template<>
		long long stov<long long>(std::string value);
		template<>
		unsigned long long stov<unsigned long long>(std::string value);
		template<>
		float stov<float>(std::string value);
		template<>
		double stov<double>(std::string value);
		template<>
		long double stov<long double>(std::string value);
		template<>
		bool stov<bool>(std::string value);
	}

	template<typename T>
	types::string to_string(T value)
	{
		return std::to_string(value);
	}

	template<>
	types::string to_string<const char*>(const char* value);
	template<>
	types::string to_string<types::string>(types::string value);
	template<>
	types::string to_string<std::string_view>(std::string_view value);

	template<class First, class Last>
	types::string to_string(First begin, Last end)
	{
		if (begin == end) return types::string();

		auto out = to_string(*begin);

		while (++begin != end)
			out += " " + to_string(*begin);

		return out;
	}

	template<class T> struct always_false : public std::false_type {};
}

#endif //HADES_TYPES_HPP
