#include "Hades/StateManager.hpp"

#include <functional>

#include "Hades/Console.hpp"
#include "Hades/ResourceManager.hpp"

namespace hades
{
	State *StateManager::getActiveState()
	{
		if(_states.empty())
			return nullptr;

		std::vector< std::unique_ptr<State> >::reverse_iterator state = _states.rbegin();

		return getValidState(state);
	}

	void StateManager::pop()
	{
		if(_states.empty())
			return;

		_states.back()->cleanup();
		_states.pop_back();
	}

	void StateManager::push(std::unique_ptr<State> &state)
	{
		state->attach([this](std::unique_ptr<hades::State>& state) {this->push(state);});
		state->attach(console);
		state->attach(resource);

		assert(_target);
		state->setGuiTarget(*_target);

		if(!_states.empty())
			_states.back()->dropFocus();

		_states.push_back(std::move(state));
	}

	void StateManager::pushUnder(std::unique_ptr<State> &state)
	{
		//push this state
		push(state);

		//then swap it with the one underneath
		std::iter_swap(_states.end(), _states.end() - 1);
	}

	void StateManager::setGuiTarget(sf::RenderTarget &target)
	{
		assert(_target == nullptr);
		_target = &target;
	}

	void StateManager::drop()
	{
		std::for_each(_states.begin(), _states.end(), [] (std::unique_ptr<hades::State> &state) {
			state->cleanup();
		});

		_states.clear();

		console = nullptr;
		resource = nullptr;
	}

	State *StateManager::getValidState(std::vector< std::unique_ptr<State> >::reverse_iterator state)
	{
		//if there are no states, return null
		if(state == _states.rend())
			return nullptr;

		//if the last state is dead, clean it up
		if(!(*state)->isAlive() && state == _states.rbegin())
		{
			(*state)->cleanup();
			pop();
			return getActiveState();
		}

		//if the last state is inactive, skip it.
		if(!(*state)->isActive())
			return getValidState(++state);

		//if this state isn't initialised, then init
		if(!(*state)->isInit())
		{
			(*state)->init();
			(*state)->initDone();
		}

		//if this state is paused, resume
		if((*state)->paused())
			(*state)->grabFocus();

		//return this state.
		return &**state;
	}
}//hades