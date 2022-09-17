#include "hades/string.hpp"

#include <array>
#include <algorithm>

namespace hades 
{
	template<>
	bool from_string<bool>(std::string_view str) noexcept
	{
		using namespace std::string_view_literals;
		constexpr auto true_str = std::array{ "true"sv, "TRUE"sv, /*"1"sv*/ };
		if (std::any_of(std::begin(true_str), std::end(true_str), [str](auto&& s) { return str == s; }))
			return true;

		// all positive values become true
		auto value = int{};
		const auto result = std::from_chars(str.data(), str.data() + str.size(), value);
		return result.ec == std::errc{} && value > 0;
	}

	template<>
	string from_string<string>(std::string_view str)
	{
		return to_string(str);
	}

	template<>
	std::string_view from_string<std::string_view>(std::string_view str) noexcept
	{
		return str;
	}
}
