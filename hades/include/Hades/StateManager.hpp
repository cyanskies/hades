#ifndef HADES_STATEMANAGER_HPP
#define HADES_STATEMANAGER_HPP

#include <memory>
#include <vector>

#include "State.hpp"

namespace sf
{
	class RenderTarget;
}

namespace hades
{
	class StateManager
	{
	public:
		StateManager() : _target(nullptr) {}

		State *getActiveState();

		//Remove state from the top of the stack
		void pop();
		//Add state to the top of the stack
		void push(std::unique_ptr<State> state);
		//Add state under the top of the stack
		//This allows the current state to finish before transfering or for a loading state to be added between the two.
		void pushUnder(std::unique_ptr<State> state);

		//Set the target to be used for Gui rendering
		void setGuiTarget(sf::RenderTarget &target);

		//deactivates the statemanager, this will drop all states, and drop utility handles to the console and resource manager
		void drop();
	private:
		using state_vector = std::vector<std::unique_ptr<State>>;
		using state_iter = state_vector::reverse_iterator;

		State *getValidState(state_iter state);
		state_vector _states;

		sf::RenderTarget *_target;
	};
}//hades

#endif //HADES_STATEMANAGER_HPP
