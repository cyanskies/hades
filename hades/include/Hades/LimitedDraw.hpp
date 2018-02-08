#ifndef HADES_LIMITED_DRAW_HPP
#define HADES_LIMITED_DRAW_HPP

#include "SFML/Graphics/Rect.hpp"

namespace hades
{
	//limits drawing to the desired area
	//NOTE: This is a performance optimisation, 
	//so the draw edge isn't required to be smooth.
	struct Camera2dDrawClamp
	{
		//specifies the 2d area within the camera
		virtual void limitDrawTo(const sf::FloatRect &worldCoords) = 0;
	};
}

#endif //HADES_LIMITED_DRAW_HPP
