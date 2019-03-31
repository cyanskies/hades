#ifndef HADES_RENDERINSTANCE_HPP
#define HADES_RENDERINSTANCE_HPP

#include "hades/export_curves.hpp"
#include "Hades/RenderInterface.hpp"
#include "hades/timers.hpp"

namespace hades
{
	class GameRenderer : public sf::Drawable, public RendererInterface
	{
	public:
		virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;

		//sets all the sprites positions and animation states
		//for the current time
		void prepareFrame();

		//sets the current game clock
		void setTime(time_point time);
		//progresses the clock by dt
		void addTime(time_duration dt);

		//insert curvestream
		void insertFrames(exported_curves import);

		//systems
		void installSystem(resources::system *system);
	private:
		//the time that should be drawn/generated
		time_point _currentTime;
	};
}

#endif //HADES_RENDERINSTANCE_HPP