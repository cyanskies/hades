#ifndef HADES_SF_COLOR_HPP
#define HADES_SF_COLOR_HPP

#include "SFML/Graphics/Color.hpp"

#include "hades/colour.hpp"

//provides conversions from sfml color objects to hades colours

namespace hades
{
	inline constexpr sf::Color to_sf_color(const colour c) noexcept
	{
		return { c.r, c.g, c.b, c.a };
	}

	inline constexpr colour from_sf_color(const sf::Color c) noexcept
	{
		return { c.r, c.g, c.b, c.a };
	}
}

#endif // !HADES_SF_COLOR_HPP
