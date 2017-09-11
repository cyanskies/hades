#ifndef HADES_SPRITE_BATCH_HPP
#define HADES_SPRITE_BATCH_HPP

#include "Hades/simple_resources.hpp"
#include "Hades/Types.hpp"

//A thread safe collection of animated sprites
//the object manages batching draw calls to render more efficiently
//the object also manages animation times.

namespace hades {
	class SpriteBatch
	{
	public:
		using sprite_id = types::uint64;
		sprite_id createAnimation();
	};
}

#endif //HADES_SPRITE_BATCH_HPP
