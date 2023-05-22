#include "hades/camera.hpp"

#include "SFML/Graphics/View.hpp"
#include "SFML/System/Vector2.hpp"

#include "hades/collision.hpp"

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
		const auto half_size = size / 2.f;

		auto pos = vector_float{ centre.x - half_size.x, centre.y - half_size.y } + move;

		const auto moved_rect = rect_float{ pos, { size.x, size.y } };
		const auto clamped_rect = clamp_rect(moved_rect, static_cast<rect_float>(rect));
		v.setCenter({ std::trunc(clamped_rect.x + half_size.x), std::trunc(clamped_rect.y + half_size.y) });
		return;
	}
}
