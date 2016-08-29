
#ifndef HADES_STATEMANAGER_HPP
#define HADES_STATEMANAGER_HPP

#include <memory>
#include <stack>


#include "State.hpp"
#include "Stitches.hpp"

namespace hades
{
	class StateManager : public State_Uses
	{
	public:
		State *getActiveState();

		void pop();
		void push(std::unique_ptr<State> &state);

		//deactivates the statemanager, this will dorp all states, and drop utility handles to the console and resource manager
		void drop();
	private:
		State *getValidState(std::vector< std::unique_ptr<State> >::reverse_iterator state);
		std::vector< std::unique_ptr<State> > _states;
	};
}//hades

#endif //HADES_STATEMANAGER_HPP
