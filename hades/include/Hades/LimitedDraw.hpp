#ifndef HADES_LIMITED_DRAW_HPP
#define HADES_LIMITED_DRAW_HPP

#include "SFML/Graphics/Rect.hpp"

namespace hades
{
	//limits drawing to the desired area
	//NOTE: This is a performance optimisation, 
	//so the draw edge isn't going to be smooth
	//use ScissorDraw to limit drawing with a smooth edge
	/*struct LimitedDraw
	{
		void SetDrawArea(const sf::FloatRect &worldCoords) = 0;
	};*/

	/*struct ScissorDraw
	{
		void SetScissorArea(const sf::FloatRect &worldCoords) = 0;
	};*/
}

#endif //HADES_LIMITED_DRAW_HPP
