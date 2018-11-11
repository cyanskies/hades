#include "hades/camera.hpp"

#include "SFML/Graphics/View.hpp"
#include "SFML/System/Vector2.hpp"

namespace hades::camera
{
	void variable_width(sf::View &v, float static_height, float height, float width)
	{
		const auto ratio = width / height;

		//set the view
		const auto size = sf::Vector2f{ static_height * ratio, static_height };
		v.setSize(size);
	}
}
