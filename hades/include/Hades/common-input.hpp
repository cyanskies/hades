#ifndef HADES_COMMON_INPUT_HPP
#define HADES_COMMON_INPUT_HPP

#include "SFML/Graphics/RenderTarget.hpp"

#include "Hades/Input.hpp"
#include "Hades/UniqueId.hpp"

namespace hades
{
	namespace input
	{
		using InputId = hades::data::UniqueId;

		extern InputId PointerPosition;
		extern InputId PointerLeft;
	}

	namespace pointer
	{
		sf::Vector2f ConvertToWorldCoords(const sf::RenderTarget &, sf::Vector2i pointer_position, const sf::View&);
		sf::Vector2i ConvertToScreenCoords(const sf::RenderTarget &, sf::Vector2f world_position, const sf::View &);
		sf::Vector2i SnapCoordsToGrid(sf::Vector2i coord, types::int32 grid_size);
	}

	//optional function for registering common input methods
	//such as mouse position and buttons:
	// mouse - mouse position
	// mouseleft - mouse button 1
	void RegisterMouseInput(InputSystem &bind);
}



#endif //!HADES_COMMON_INPUT_HPP