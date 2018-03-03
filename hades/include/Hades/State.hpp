#ifndef HADES_STATE_HPP
#define HADES_STATE_HPP

#include <atomic>

#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Window/Joystick.hpp"
#include "SFML/Window/Window.hpp"

#include "SFGUI/Desktop.hpp"

#include "Hades/Input.hpp"

namespace hades
{
	class State
	{
	public:
		State();
		virtual ~State();

		using push_func = std::function<void(std::unique_ptr<State>)>;

		void setStateManangerCallbacks(push_func push, push_func push_under);

		//functions to ditermine whether the state should die
		bool isAlive() const;

		bool isInit() const;
		void initDone();

		//manually kill it from within the state.
		void kill();

		//pauses state
		void dropFocus();
		
		//tests if the state currently has focus
		bool paused() const;
		//resumes state
		void grabFocus();
		
		//keep the gui working
		bool guiInput(sf::Event&);
		void updateGui(sf::Time frameTime);

		//functions for states to overide to define behaviour
		//main state loop
		virtual void init() = 0; //start all the state logic, this is for setting up game state
		virtual bool handleEvent(const Event&) = 0; //handle any events you want
		//tick game state with variable rate
		//advance the game simulation by deltaTime ms
		virtual void update(sf::Time deltaTime, const sf::RenderTarget&, InputSystem::action_set) = 0;
		//update animations and draw
		//dtime is the last time since draw was called
		//draw the game at the previous draw time + deltaTime
		virtual void draw(sf::RenderTarget &target, sf::Time deltaTime) = 0; //draw
		virtual void cleanup() = 0; //delete everything
		
		//support functions
		virtual void reinit() = 0; //reinit because graphcs options changed, or state has been paused
		virtual void pause() = 0; //pause any custom timers you have, this state is no longer active/being drawn/reciveing input
		virtual void resume() = 0; //restart any custom timers, this state is active again and being drawn, recieving input
	
	protected:
		sfg::Desktop _gui;

		push_func PushState, PushStateUnder;

	private:
		std::atomic_bool _alive, _init, _paused;
	};
}//hades

#endif //HADES_STATE_HPP