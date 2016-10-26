#ifndef HADES_TYPES_HPP
#define HADES_TYPES_HPP

#include <string>
#include <type_traits>

#include "SFML/Config.hpp"

namespace hades {
	namespace types {
		//portable types for integers
		using int8 = sf::Int8;
		using uint8 = sf::Uint8;

		using int16 = sf::Int16;
		using uint16 = sf::Uint16;

		using int32 = sf::Int32;
		using uint32 = sf::Uint32;

		using int64 = sf::Int64;
		using uint64 = sf::Uint64;

		//type trait to ditermine if types should be used in hades
		//console vars, entity components and curves must be checked by this type

		//test if(hades_type<T>::value == true)
		//approved types are float, double, long double, string, the ints above
		template<typename T>
		struct hades_type : std::false_type {};

		template<>
		struct hades_type<float> : std::is_same<float, typename std::remove_cv<float>::type> {};

		//float64
		template<>
		struct hades_type<double> : std::is_same<double, typename std::remove_cv<double>::type> {};

		//float80
		template<>
		struct hades_type<long double> : std::is_same<long double, typename std::remove_cv<long double>::type> {};

		//accepted integer types
		template<>
		struct hades_type<int8> : std::is_same<int8, typename std::remove_cv<int8>::type> {};

		template<>
		struct hades_type<uint8> : std::is_same<uint8, typename std::remove_cv<uint8>::type> {};

		template<>
		struct hades_type<int16> : std::is_same<int16, typename std::remove_cv<int16>::type> {};

		template<>
		struct hades_type<uint16> : std::is_same<uint16, typename std::remove_cv<uint16>::type> {};

		template<>
		struct hades_type<int32> : std::is_same<int32, typename std::remove_cv<int32>::type> {};

		template<>
		struct hades_type<uint32> : std::is_same<uint32, typename std::remove_cv<uint32>::type> {};

		template<>
		struct hades_type<int64> : std::is_same<int64, typename std::remove_cv<int64>::type> {};

		template<>
		struct hades_type<uint64> : std::is_same<uint64, typename std::remove_cv<uint64>::type> {};

		//bool types
		template<>
		struct hades_type<bool> : std::is_same<bool, typename std::remove_cv<bool>::type> {};

		//string types
		template<>
		struct hades_type<std::string> : std::is_same<std::string, typename std::remove_cv<std::string>::type> {};

		template<typename T>
		using approved_types = hades_type<T>;

		//string to value conversion
		template<typename T>
		T stov(std::string value)
		{
			static_assert(false, "Tried to convert type not covered by stov");
		}

		// type conversion
		template<>
		int8 stov<int8>(std::string value);
		template<>
		uint8 stov<uint8>(std::string value);
		template<>
		int16 stov<int16>(std::string value);
		template<>
		uint16 stov<uint16>(std::string value);
		template<>
		int32 stov<int32>(std::string value);
		template<>
		uint32 stov<uint32>(std::string value);
		template<>
		int64 stov<int64>(std::string value);
		template<>
		uint64 stov<uint64>(std::string value);
		template<>
		float stov<float>(std::string value);
		template<>
		double stov<double>(std::string value);
		template<>
		long double stov<long double>(std::string value);
		template<>
		bool stov<bool>(std::string value);

		namespace names {
			//type ids
			enum HadesType {
				INT8, UINT8,
				INT16, UINT16,
				INT32, UINT32,
				INT64, UINT64,
				FLOAT, DOUBLE, LONGDOUBLE,
				BOOL,
				STRING, FUNCTION,
				ERROR_TYPE
			};
		}

		//Type ditermination for storage
		template<typename T>
		names::HadesType Type() { return ERROR_TYPE; }
	}
}

#endif //HADES_TYPES_HPP