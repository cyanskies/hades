#ifndef HADES_RENDERINTERFACE_HPP
#define HADES_RENDERINTERFACE_HPP

#include "hades/level_interface.hpp"
#include "hades/SpriteBatch.hpp"

namespace hades
{
	class RendererInterface : public GameInterface
	{
	public:
		//controls for messing with sprites
		SpriteBatch *getSprites();
	protected:
		SpriteBatch _sprites;
	};
}

#endif //HADES_RENDERINTERFACE_HPP
