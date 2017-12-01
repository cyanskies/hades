#include "Hades/common-input.hpp"

namespace hades
{
	namespace input
	{
		InputId PointerPosition = InputId::Zero;
		InputId PointerLeft = InputId::Zero;
	}

	void RegisterMouseInput(InputSystem &bind)
	{
		auto data = data_manager;
		//collect unique names for actions
		input::PointerPosition = data->getUid("mouse");
		input::PointerLeft = data->getUid("mouseleft");

		//bind them
		bind.create(input::PointerPosition, false, "mouse");
		bind.create(input::PointerLeft, false, "mouseleft");
	}
}