#ifndef HADES_COLOUR_HPP
#define HADES_COLOUR_HPP

#include <limits>

#include "hades/types.hpp"

namespace hades
{
	struct colour
	{
		colour() noexcept = default;
		~colour() noexcept = default;

		colour(const colour&) noexcept = default;
		colour &operator=(const colour&) noexcept = default;

		uint8 r{},
			g{},
			b{},
			a{std::numeric_limits<uint8>::max()};
	};
}

#endif //!HADES_COLOUR_HPP