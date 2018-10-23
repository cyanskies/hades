#ifndef HADES_STATEMANAGER_HPP
#define HADES_STATEMANAGER_HPP

#include <memory>
#include <vector>

#include "hades/state.hpp"

namespace sf
{
	class RenderTarget;
}

namespace hades
{
	class StateManager
	{
	public:
		State *getActiveState();

		//Remove state from the top of the stack
		void pop();
		//Add state to the top of the stack
		void push(std::unique_ptr<state> state);
		//Add state under the top of the stack
		//This allows the current state to finish before transfering or for a loading state to be added between the two.
		void pushUnder(std::unique_ptr<state> state);

		//deactivates the statemanager, this will drop all states, and drop utility handles to the console and resource manager
		void drop();
	private:
		using state_vector = std::vector<std::unique_ptr<state>>;
		using state_iter = state_vector::reverse_iterator;

		state *getValidState(state_iter state);
		state_vector _states;
	};
}//hades

#endif //HADES_STATEMANAGER_HPP
