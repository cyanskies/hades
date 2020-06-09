#ifndef HADES_COLOUR_HPP
#define HADES_COLOUR_HPP

#include <array>
#include <cassert>
#include <limits>

#include "hades/types.hpp"
#include "hades/utility.hpp"

namespace hades
{
	struct colour
	{
		using value_type = uint8;

		uint8& operator[](std::size_t i) noexcept
		{
			constexpr auto arr = std::array{ &colour::r, &colour::g, &colour::b, &colour::a };
			assert(i < size(arr));
			return std::invoke(arr[i], this);
		}

		const uint8& operator[](std::size_t i) const noexcept
		{
			constexpr auto arr = std::array{ &colour::r, &colour::g, &colour::b, &colour::a };
			assert(i < size(arr));
			return std::invoke(arr[i], this);
		}

		uint8 r{},
			g{},
			b{},
			a{std::numeric_limits<uint8>::max()};
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
	template<>
	string to_string<colour>(colour);

	namespace colours
	{
		constexpr auto black = colour{};
		constexpr auto white = colour{ std::numeric_limits<uint8>::max(),
									   std::numeric_limits<uint8>::max(),
									   std::numeric_limits<uint8>::max() };

		constexpr auto transparent = colour{ std::numeric_limits<uint8>::max(),
											 std::numeric_limits<uint8>::max(),
											 std::numeric_limits<uint8>::max(),
											 0u };
	}
}

#endif //!HADES_COLOUR_HPP