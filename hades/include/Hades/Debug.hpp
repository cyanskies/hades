#ifndef HADES_DEBUG_HPP
#define HADES_DEBUG_HPP

#include <list>
#include <memory>

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/System/Vector2.hpp"

#include "Hades/drawable.hpp"

namespace hades
{
	namespace debug
	{
		class Overlay : public drawable
		{
		public:
			explicit Overlay(bool fullscreen = false);

			virtual ~Overlay();
			//returns the requested size of the overlay,
			//this allows the debug manager to arrange the overlays correctly
			//NOTE: failing to override this function if fullscreen == false, will prevent your
			//overlay from being rendered
			virtual sf::Vector2f getSize() const;

			//if fullscreen is true, then this is called on creation, and 
			//any time the window size changes
			//NOTE: failing to override this function if fullscreen == true will
			//prevent your overlay from being rendered
			virtual void setFullscreenSize(sf::Vector2f);

			virtual void update();
			//draws the overlay to the screen.
			//if fullscreen is true, then the overlay must provide it's own view
			//otherwise the view is already arranged for the overlay
			//and is the size returned by getSize()
			void draw(time_duration, sf::RenderTarget&, sf::RenderStates = {}) override = 0;

			//if fullscreen then the size is specified by the window manager
			bool fullscreen() const;
			bool valid() const;

		private:
			bool _fullscreen;
			mutable bool _invalid;
		};

		class OverlayManager : public drawable
		{
		public:
			Overlay* createOverlay(std::unique_ptr<Overlay>);
			Overlay* destroyOverlay(Overlay*);

			void update();
			void draw(time_duration, sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates()) override;

			void setWindowSize(sf::Vector2u);

		private:
			sf::Vector2f _windowSize;
			std::list<std::unique_ptr<Overlay>> _overlays;
		};

		extern OverlayManager *overlay_manager;

		//adds an overlay to the overlay list
		Overlay *CreateOverlay(std::unique_ptr<Overlay> overlay);
		//removes an overlay from the overlay list
		//sets the passed overlay ptr to nullptr if successful
		Overlay* DestroyOverlay(Overlay*);
	}
}

#endif //HADES_DEBUG_HPP
