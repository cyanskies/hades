#include "hades/string.hpp"

#include <array>
#include <algorithm>

namespace hades 
{
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
}
