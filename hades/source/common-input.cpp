#include "Hades/common-input.hpp"

#include "Hades/Data.hpp"

namespace hades
{
	namespace input
	{
		InputId PointerPosition = InputId::zero;
		InputId PointerLeft = InputId::zero;
	}

	namespace pointer
	{
		sf::Vector2f ConvertToWorldCoords(const sf::RenderTarget &t, sf::Vector2i pointer_position, const sf::View &v)
		{
			return t.mapPixelToCoords(pointer_position, v);
		}

		sf::Vector2i ConvertToScreenCoords(const sf::RenderTarget &t, sf::Vector2f world_position, const sf::View &v)
		{
			return t.mapCoordsToPixel(world_position, v);
		}

		sf::Vector2i SnapCoordsToGrid(sf::Vector2i coord, types::int32 grid_size)
		{
			const auto coordf = static_cast<sf::Vector2f>(coord);
			
			const auto snapPos = coordf - sf::Vector2f(static_cast<float>(std::abs(std::fmod(coordf.x, grid_size))),
				static_cast<float>(std::abs((std::fmod(coordf.y, grid_size)))));

			return static_cast<sf::Vector2i>(snapPos);
		}
	}

	void RegisterMouseInput(InputSystem &bind)
	{
		//collect unique names for actions
		input::PointerPosition = data::MakeUid("mouse");
		input::PointerLeft = data::MakeUid("mouseleft");

		//bind them
		bind.create(input::PointerPosition, false, "mouse");
		bind.create(input::PointerLeft, false, "mouseleft");
	}
}