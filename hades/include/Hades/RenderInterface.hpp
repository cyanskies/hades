#ifndef HADES_RENDERINTERFACE_HPP
#define HADES_RENDERINTERFACE_HPP

#include "hades/level_interface.hpp"
#include "hades/sprite_batch.hpp"

namespace hades
{
	class RendererInterface : public GameInterface
	{
	public:
		//controls for messing with sprites
		sprite_batch  *getSprites();
	protected:
		sprite_batch  _sprites;
	};
}

#endif //HADES_RENDERINTERFACE_HPP
