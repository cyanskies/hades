#ifndef HADES_STATE_HPP
#define HADES_STATE_HPP

#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Window/Joystick.hpp"
#include "SFML/Window/Window.hpp"

#include "TGUI/Gui.hpp"

#include "Bind.hpp"
#include "Stitches.hpp"
#include "TimerManager.hpp"

namespace hades
{
	class State : public State_Uses
	{
	public:
		State();
		virtual ~State();

		//functions to ditermine whether the state should die
		bool isAlive() const;

		bool isInit() const;
		void initDone();

		//manually kill it from within the state.
		void kill();

		//tests if the state wants focus //
		//calls resume
		void activate();

		bool isActive() const;
		//pauses state and skips it from now on
		void deactivate();

		void dropFocus();
		
		//tests if the state currently has focus
		bool paused() const;
		//pauses game timers and tells the app not to run logic updates for this state
		void grabFocus();
		
		CallbackSystem &getCallbackHandler();
		void handleInput(thor::ActionContext<int> context, int id);

		void setGuiTarget(sf::RenderTarget &target);
		void drawGui();

		//functions for states to overide to define behaviour

		//main state loop
		virtual void init() = 0; //start all the state logic
		virtual bool handleEvent(sf::Event &windowEvent) = 0; //handle any events you want
		//virtual void input(int input) = 0;
		[[depreciated("Constant update is depreciated, use update(dtime) instead.")]]
		virtual void update(const sf::Window &window) = 0; //update at constant tickrate
		virtual void update(sf::Time deltaTime) = 0; //update animations
		//TODO: draw with dtime
		virtual void draw(sf::RenderTarget &target) = 0; //draw
		virtual void cleanup() = 0; //delete everything
		
		//support functions
		virtual void reinit() = 0; //reinit because graphcs options changed
		virtual void pause() = 0; //pause any custom timers you have
		virtual void resume() = 0; //restart any custom timers
	
	protected:
		void bindCallback(int id, std::function<void(EventContext)> func);
		void clearCallback(int id);
		void clearCallbacks();

		tgui::Gui _gui;

	private:
		//TODO: redo input system
		CallbackSystem _callbacks;
		std::map<int, std::function<void(thor::ActionContext<int>)> > _stateBinds;

		//TODO: atomic
		bool _alive, _init, _paused, _active;
	};
}//hades

#endif //HADES_STATE_HPP