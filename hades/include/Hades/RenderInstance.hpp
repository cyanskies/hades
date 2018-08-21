#ifndef HADES_RENDERINSTANCE_HPP
#define HADES_RENDERINSTANCE_HPP

#include "Hades/ExportedCurves.hpp"
#include "Hades/RenderInterface.hpp"

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
		void setTime(sf::Time time);
		//progresses the clock by dt
		void addTime(sf::Time dt);

		//insert curvestream
		void insertFrames(ExportedCurves import);

		//systems
		void installSystem(resources::system *system);
	private:
		//the time that should be drawn/generated
		sf::Time _currentTime;
	};
}

#endif //HADES_RENDERINSTANCE_HPP