#ifndef HADES_CAMERA_HPP
#define HADES_CAMERA_HPP

#include "hades/rectangle_math.hpp"
#include "hades/vector_math.hpp"

namespace sf
{
	class View;
}

namespace hades::camera
{
	constexpr float calculate_width(float static_height, float width, float height) noexcept
	{
		const auto ratio = width / height;
		return static_height * ratio;
	}

	void variable_width(sf::View&, float static_height, float width, float height) noexcept;
	void move(sf::View&, vector2_float, rect_int) noexcept;
}

#endif //!HADES_CAMERA_HPP
