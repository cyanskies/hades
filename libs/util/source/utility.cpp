#include "hades/utility.hpp"

namespace hades {
	std::string_view trim(std::string_view in)
	{
		const auto start = in.find_first_not_of(' ');
		const auto end = in.find_last_not_of(' ');

		if (in.empty())
			return {};

		return in.substr(start, end - start + 1);
	}

	string to_string(const char* value)
	{
		return string{ value };
	}

	string to_string(string value)
	{
		return value;
	}

	string to_string(std::string_view value)
	{
		return types::string{ value };
	}

	template<>
	bool from_string<bool>(std::string_view str) noexcept
	{
		using namespace std::string_view_literals;
		constexpr auto true_str = std::array{ "true"sv, "TRUE"sv, "1"sv };
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
