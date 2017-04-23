#include "Hades/StateManager.hpp"

#include <functional>

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

		_states.back()->cleanup();
		_states.pop_back();
	}

	void StateManager::push(std::unique_ptr<State> state)
	{
		state->setStateManangerCallbacks([this](std::unique_ptr<hades::State> state) {this->push(std::move(state));},
			[this](std::unique_ptr<hades::State> state) {this->pushUnder(std::move(state)); });

		assert(_target);
		state->setGuiTarget(*_target);

		if(!_states.empty())
			_states.back()->dropFocus();

		_states.push_back(std::move(state));
	}

	void StateManager::pushUnder(std::unique_ptr<State> state)
	{
		//push this state
		push(std::move(state));

		//this will end up being under the current state, so it won't have focus
		state->dropFocus();

		//then swap it with the one underneath
		std::iter_swap(_states.rbegin(), _states.rbegin() + 1);
	}

	void StateManager::setGuiTarget(sf::RenderTarget &target)
	{
		assert(_target == nullptr);
		_target = &target;

		//reset gui target for all current states
		for (auto &s : _states)
			s->setGuiTarget(target);
	}

	void StateManager::drop()
	{
		for (auto &s : _states)
			s->cleanup();

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
		}

		//if this state is paused, resume
		if ((*state)->paused())
		{
			(*state)->grabFocus();
			(*state)->reinit();
		}

		//return this state.
		return &**state;
	}
}//hades