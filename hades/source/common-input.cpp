#include "Hades/common-input.hpp"

#include "Hades/Data.hpp"

namespace hades
{
	namespace input
	{
		InputId PointerPosition = InputId::Zero;
		InputId PointerLeft = InputId::Zero;
	}

	void RegisterMouseInput(InputSystem &bind)
	{
		//collect unique names for actions
		input::PointerPosition = data::GetUid("mouse");
		input::PointerLeft = data::GetUid("mouseleft");

		//bind them
		bind.create(input::PointerPosition, false, "mouse");
		bind.create(input::PointerLeft, false, "mouseleft");
	}
}