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

	void StateManager::push(std::unique_ptr<State> state)
	{
		state->setStateManangerCallbacks([this](std::unique_ptr<hades::State> state) {this->push(std::move(state));},
			[this](std::unique_ptr<hades::State> state) {this->pushUnder(std::move(state)); });
		
		if(!_states.empty())
			_states.back()->dropFocus();

		_states.push_back(std::move(state));
	}

	void StateManager::pushUnder(std::unique_ptr<State> state)
	{
		//this will end up being under the current state, so it won't have focus
		state->dropFocus();

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
		if(!(*state)->isAlive())
		{
			pop();
			return getActiveState();
		}

		//if this state isn't initialised, then init
		if(!(*state)->isInit())
		{
			(*state)->init();
			(*state)->initDone();
			//make sure the gui has the correct view size
			auto w = console::GetInt("vid_width", 800), h = console::GetInt("vid_height", 600);
		}

		//if this state is paused, resume
		if ((*state)->paused())
		{
			(*state)->grabFocus();
			(*state)->reinit();
			//make sure the gui has the correct view size
			auto w = console::GetInt("vid_width", 800), h = console::GetInt("vid_height", 600);
		}

		//return this state.
		return &**state;
	}
}//hades