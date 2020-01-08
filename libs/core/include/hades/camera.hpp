#ifndef HADES_CAMERA_HPP
#define HADES_CAMERA_HPP

namespace sf
{
	class View;
}

namespace hades::camera
{
	void variable_width(sf::View&, float static_height, float width, float height);
}

#endif //!HADES_CAMERA_HPP