#ifndef HADES_STATE_HPP
#define HADES_STATE_HPP

#include <atomic>
#include <functional>

#include "SFML/Graphics/RenderTarget.hpp"

#include "hades/input.hpp"
#include "hades/timers.hpp"
#include "hades/sf_input.hpp"

namespace hades
{
	class state
	{
	public:
		virtual ~state() noexcept = default;

		template<typename Push, typename PushUnder>
		void state_manager_callbacks(Push push, PushUnder push_under);

		//functions to ditermine whether the state should die
		bool is_alive() const;
		bool is_init() const;

		//[internal] called by the app to log that the state has already had init called
		void init_done();

		//manually kill it from within the state.
		void kill();

		//pauses state
		void drop_focus();
		
		//tests if the state currently has focus
		bool paused() const;
		//resumes state
		void grab_focus();
		
		//functions for states to overide to define behaviour
		//main state loop
		virtual void init() { reinit(); } //start all the state logic, this is for setting up game state
		virtual bool handle_event(const event&) { return false;  } //handle any events you want, unhandled events go through the input system
		//tick game state with variable rate
		//advance the game simulation by deltaTime ms
		virtual void update(time_duration, const sf::RenderTarget&, input_system::action_set) {}
		
		//update animations and draw
		//dtime is the last time since draw was called
		//draw the game at the previous draw time + deltaTime
		virtual void draw(sf::RenderTarget &target, time_duration delta_time) {}

		//support functions
		virtual void reinit() {} //reinit because graphcs options changed, or state has been paused
		virtual void pause() {} //pause any custom timers you have, this state is no longer active/being drawn/reciveing input
		virtual void resume() {} //restart any custom timers, this state is active again and being drawn, recieving input
	
	protected:
		using push_func = std::function<void(std::unique_ptr<state>)>;
		push_func push_state, push_state_under;

	private:
		std::atomic_bool _alive = true, _init = false, _paused = false;
	};
}//hades

#include "hades/detail/state.inl"

#endif //HADES_STATE_HPP