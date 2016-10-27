#include "Hades/Types.hpp"

#include <limits>

namespace hades {
	namespace types {
		// type conversion
		template<>
		int8 stov<int8>(std::string value)
		{
			int32 val = std::stoi(value);

			if (val <= std::numeric_limits<int8>::min() || val >= std::numeric_limits<int8>::max())
				throw std::out_of_range("stov argument out of range");

			return static_cast<int8>(val);
		}

		template<>
		uint8 stov<uint8>(std::string value)
		{
			int32 val = std::stoi(value);

			if (val <= std::numeric_limits<uint8>::min() || val >= std::numeric_limits<uint8>::max())
				throw std::out_of_range("stov argument out of range");

			return static_cast<uint8>(val);
		}

		template<>
		int16 stov<int16>(std::string value)
		{
			int32 val = std::stoi(value);

			if (val <= std::numeric_limits<int16>::min() || val >= std::numeric_limits<int16>::max())
				throw std::out_of_range("stov argument out of range");

			return static_cast<int16>(val);
		}

		template<>
		uint16 stov<uint16>(std::string value)
		{
			int32 val = std::stoi(value);

			if (val <= std::numeric_limits<uint16>::min() || val >= std::numeric_limits<uint16>::max())
				throw std::out_of_range("stov argument out of range");

			return static_cast<uint16>(val);
		}

		template<>
		int32 stov<int32>(std::string value)
		{
			return std::stoi(value);
		}

		template<>
		uint32 stov<uint32>(std::string value)
		{
			int64 val = std::stoll(value);

			if (val <= std::numeric_limits<uint32>::min() || val >= std::numeric_limits<uint32>::max())
				throw std::out_of_range("stov argument out of range");

			return static_cast<uint32>(val);
		}

		template<>
		int64 stov<int64>(std::string value)
		{
			return std::stoll(value);
		}

		template<>
		uint64 stov<uint64>(std::string value)
		{
			return std::stoull(value);
		}

		template<>
		float stov<float>(std::string value)
		{
			return std::stof(value);
		}

		template<>
		double stov<double>(std::string value)
		{
			return std::stod(value);
		}

		template<>
		long double stov<long double>(std::string value)
		{
			return std::stold(value);
		}

		template<>
		bool stov<bool>(std::string value)
		{
			return value != "0";
		}

		using namespace names;

		//floats
		template<>
		HadesType Type<float>() { return FLOAT; }

		template<>
		HadesType Type<double>() { return DOUBLE; }

		template<>
		HadesType Type<long double>() { return LONGDOUBLE; }

		//integers
		template<>
		HadesType Type<int8>() { return INT8; }

		template<>
		HadesType Type<uint8>() { return UINT8; }

		template<>
		HadesType Type<int16>() { return INT16; }

		template<>
		HadesType Type<uint16>() { return UINT16; }

		template<>
		HadesType Type<int32>() { return INT32; }

		template<>
		HadesType Type<uint32>() { return UINT32; }

		template<>
		HadesType Type<int64>() { return INT64; }

		template<>
		HadesType Type<uint64>() { return UINT64; }

		template<>
		HadesType Type<bool>() { return BOOL; }

		template<>
		HadesType Type<std::string>() { return STRING; }
	}
}