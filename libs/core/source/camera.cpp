#include "hades/camera.hpp"

#include "SFML/Graphics/View.hpp"
#include "SFML/System/Vector2.hpp"

namespace hades::camera
{
	void variable_width(sf::View &v, float static_height, float width, float height) noexcept
	{
		const auto ratio = width / height;

		//set the view
		const auto size = sf::Vector2f{ static_height * ratio, static_height };
		v.setSize(size);
	}

	void move(sf::View& v, const vector_float move, const rect_int rect) noexcept
	{
		const auto centre = v.getCenter();
		const auto size = v.getSize();

		auto pos = vector_float{ centre.x - size.x / 2, centre.y - size.y / 2 } + move;

		//keep view inside rect area
		if (pos.x < rect.x)
			pos.x = static_cast<float>(rect.x);
		if (pos.y < rect.y)
			pos.y = static_cast<float>(rect.y);
		if (pos.x + size.x > rect.x + rect.width)
			pos.x = rect.x + rect.width - size.x;
		if (pos.y + size.y > rect.y + rect.height)
			pos.y = rect.y + rect.height - size.y;

		//if view is larger than rect
		//centre it
		if (size.x > rect.width)
			pos.x = size.x / 2 - (rect.x + rect.width / 2);
		if (size.y > rect.height)
			pos.y = size.y / 2 - (rect.y + rect.height / 2);

		//round to pixel pos
		v.reset({ {std::floor(pos.x), std::floor(pos.y)}, size });
	}
}
