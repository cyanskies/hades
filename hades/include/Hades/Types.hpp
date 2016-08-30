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

		//string to value conversion
		template<typename T>
		T stov(std::string value)
		{
			static_assert(false, "Tried to convert type not covered by stov");
		}

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

		//Type ditermination for storage
		template<typename T>
		HadesType Type() { return ERROR_TYPE; }
	}
}

#include "detail/Types.inl"

#endif //HADES_TYPES_HPP
