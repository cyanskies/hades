#include "Hades/Types.hpp"

#include <limits>

namespace hades {
	namespace types {
		// type conversion
		template<>
		signed char stov<signed char>(std::string value)
		{
			int val = std::stoi(value);

			if (val <= std::numeric_limits<signed char>::min() || val >= std::numeric_limits<signed char>::max())
				throw std::out_of_range("stov argument out of range");

			return static_cast<signed char>(val);
		}

		template<>
		unsigned char stov<unsigned char>(std::string value)
		{
			int val = std::stoi(value);

			if (val <= std::numeric_limits<unsigned char>::min() || val >= std::numeric_limits<unsigned char>::max())
				throw std::out_of_range("stov argument out of range");

			return static_cast<unsigned char>(val);
		}

		template<>
		short stov<short>(std::string value)
		{
			int val = std::stoi(value);

			if (val <= std::numeric_limits<short>::min() || val >= std::numeric_limits<short>::max())
				throw std::out_of_range("stov argument out of range");

			return static_cast<short>(val);
		}

		template<>
		unsigned short stov<unsigned short>(std::string value)
		{
			int val = std::stoi(value);

			if (val <= std::numeric_limits<unsigned short>::min() || val >= std::numeric_limits<unsigned short>::max())
				throw std::out_of_range("stov argument out of range");

			return static_cast<unsigned short>(val);
		}

		template<>
		int stov<int>(std::string value)
		{
			return std::stoi(value);
		}

		template<>
		unsigned int stov<unsigned int>(std::string value)
		{
			long long val = std::stoll(value);

			if (val <= std::numeric_limits<unsigned int>::min() || val >= std::numeric_limits<unsigned int>::max())
				throw std::out_of_range("stov argument out of range");

			return static_cast<unsigned int>(val);
		}

		template<>
		long long stov<long long>(std::string value)
		{
			return std::stoll(value);
		}

		template<>
		unsigned long long stov<unsigned long long>(std::string value)
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
			return value != "0" && value != "false";
		}
	}

	template<>
	types::string to_string<const char*>(const char* value)
	{
		return types::string(value);
	}

	template<>
	types::string to_string<types::string>(types::string value)
	{
		return value;
	}

	template<>
	types::string to_string<std::string_view>(std::string_view value)
	{
		return types::string(value);
	}
}
