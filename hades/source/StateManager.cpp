#include "Hades/StateManager.hpp"

//#include <functional>

#include "Hades/Properties.hpp"

namespace hades
{
	State *StateManager::getActiveState()
	{
		if(_states.empty())
			return nullptr;

		state_iter state = _states.rbegin();

		return getValidState(state);
	}

	void StateManager::pop()
	{
		if(_states.empty())
			return;

		_states.pop_back();
	}

	void StateManager::push(std::unique_ptr<state> state)
	{
		state->state_manager_callbacks([this](std::unique_ptr<hades::state> state) {this->push(std::move(state));},
			[this](std::unique_ptr<hades::state> state) {this->pushUnder(std::move(state)); });
		
		if(!_states.empty())
			_states.back()->drop_focus();

		_states.push_back(std::move(state));
	}

	void StateManager::pushUnder(std::unique_ptr<state> state)
	{
		//this will end up being under the current state, so it won't have focus
		state->drop_focus();

		//push this state
		push(std::move(state));

		//then swap it with the one underneath
		std::iter_swap(_states.rbegin(), _states.rbegin() + 1);
	}

	void StateManager::drop()
	{
		_states.clear();
	}

	State *StateManager::getValidState(StateManager::state_iter state)
	{
		//if the state is dead, clean it up and loop though to the next state
		if(!(*state)->is_alive())
		{
			pop();
			return getActiveState();
		}

		//if this state isn't initialised, then init
		if(!(*state)->is_init())
		{
			(*state)->init();
			(*state)->init_done();
			//make sure the gui has the correct view size
			auto w = console::get_int("vid_width", 800), h = console::get_int("vid_height", 600);
		}

		//if this state is paused, resume
		if ((*state)->paused())
		{
			(*state)->grab_focus();
			(*state)->reinit();
			//make sure the gui has the correct view size
			auto w = console::get_int("vid_width", 800), h = console::get_int("vid_height", 600);
		}

		//return this state.
		return &**state;
	}
}//hades