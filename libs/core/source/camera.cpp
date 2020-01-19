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

		auto pos = vector_float{ centre.x - size.x / 2, centre.y - size.y / 2 } + move;

		const auto moved_rect = rect_float{ pos, { size.x, size.y } };
		const auto clamped_rect = clamp_rect(moved_rect, static_cast<rect_float>(rect));

		//round to pixel pos
		v.reset({ {std::floor(clamped_rect.x), std::floor(clamped_rect.y)}, size });
		return;
	}
}
