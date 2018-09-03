#include "hades/utility.hpp"

namespace hades {
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
