#ifndef HADES_DRAWABLE_HPP
#define HADES_DRAWABLE_HPP

#include "SFML/Graphics/RenderStates.hpp"

#include "hades/time.hpp"

namespace sf
{
	class RenderTarget;
}

namespace hades
{
	class drawable
	{
	public:
		virtual ~drawable() noexcept = default;

		//sfml draw, plus time since last call to draw
		virtual void draw(time_duration, sf::RenderTarget&, sf::RenderStates = {}) = 0;
	};
}

#endif //!HADES_DRAWABLE_HPP
