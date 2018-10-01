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

	template<>
	bool from_string<bool>(std::string_view str)
	{
		using namespace std::string_view_literals;
		static const auto true_str = { "true"sv, "TRUE"sv, "1"sv };
		return std::any_of(std::begin(true_str), std::end(true_str), [str](auto &&s) { return str == s; });
	}

	template<>
	string from_string<string>(std::string_view str)
	{
		return to_string(str);
	}

	template<>
	std::string_view from_string<std::string_view>(std::string_view str)
	{
		return str;
	}
}
