#ifndef HADES_COMMON_INPUT_HPP
#define HADES_COMMON_INPUT_HPP

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

	//optional function for registering common input methods
	//such as mouse position and buttons:
	// mouse - mouse position
	// mouseleft - mouse button 1
	void RegisterMouseInput(InputSystem &bind);
}



#endif //!HADES_COMMON_INPUT_HPP