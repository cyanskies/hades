#ifndef HADES_COLOUR_HPP
#define HADES_COLOUR_HPP

#include <array>
#include <cassert>
#include <limits>
#include <string_view>

#include "hades/string.hpp"
#include "hades/types.hpp"

//https://www.color-hex.com/ has shade tables for hex colours

namespace hades
{
	struct colour
	{
		using value_type = uint8;

		constexpr value_type& operator[](std::size_t i) noexcept
		{
			constexpr auto arr = std::array{ &colour::r, &colour::g, &colour::b, &colour::a };
			assert(i < size(arr));
			return std::invoke(arr[i], this);
		}

		constexpr const value_type& operator[](std::size_t i) const noexcept
		{
			constexpr auto arr = std::array{ &colour::r, &colour::g, &colour::b, &colour::a };
			assert(i < size(arr));
			return std::invoke(arr[i], this);
		}

		value_type r = 0;
		value_type g = 0;
		value_type b = 0;
		value_type a = 255;
	};

	constexpr bool operator==(colour l, colour r) noexcept
	{
		return std::tie(l[0], l[1], l[2], l[3]) == std::tie(r[0], r[1], r[2], r[3]);
	}

	constexpr bool operator!=(colour l, colour r) noexcept
	{
		return !(l == r);
	}

	constexpr bool operator<(colour l, colour r) noexcept
	{
		return std::tie(l[0], l[1], l[2], l[3]) < std::tie(r[0], r[1], r[2], r[3]);
	}

	template<>
	colour from_string<colour>(std::string_view);
	string to_string(colour);

	namespace colours
	{
		constexpr auto black = colour{};
		constexpr auto white = colour{ std::numeric_limits<colour::value_type>::max(),
									   std::numeric_limits<colour::value_type>::max(),
									   std::numeric_limits<colour::value_type>::max() };

		constexpr auto transparent = colour{ std::numeric_limits<colour::value_type>::max(),
											 std::numeric_limits<colour::value_type>::max(),
											 std::numeric_limits<colour::value_type>::max(),
											 0u };

		constexpr auto off_lime = colour{ 186, 218, 85 };
		constexpr auto mute_cyan = colour{ 127, 229, 240 };
		constexpr auto red = colour{ 255, 0, 0 };
		constexpr auto lavender = colour{ 255, 128, 237 };
		constexpr auto dark_blue = colour{ 64, 114, 148 };
		constexpr auto dark_cream = colour{ 203, 203, 169 };
		constexpr auto lawn_green = colour{ 90, 193, 142 };
		constexpr auto orange = colour{ 255, 165, 0 };
		constexpr auto gold = colour{ 255, 215, 0 };
		constexpr auto turquoise = colour{ 64, 224, 208 };

		using namespace std::string_view_literals;
		constexpr auto all_colour_names = std::array{ "off_lime"sv, "mute_cyan"sv, "red"sv, "lavender"sv,
			"dark-blue"sv, "dark-cream"sv, "lawn-green"sv, "orange"sv, "gold"sv, "turquoise"sv, "black"sv, "white"sv };
		constexpr auto all_colours = std::array{ off_lime, mute_cyan, red, lavender,
			dark_blue, dark_cream, lawn_green, orange, gold, turquoise, black, white };
	}
}

#endif //!HADES_COLOUR_HPP