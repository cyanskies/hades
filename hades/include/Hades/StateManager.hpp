
#ifndef HADES_STATEMANAGER_HPP
#define HADES_STATEMANAGER_HPP

#include <memory>
#include <stack>

#include "State.hpp"
#include "Stitches.hpp"

namespace sf
{
	class RenderTarget;
}

namespace hades
{
	class StateManager : public State_Uses
	{
	public:
		StateManager() : _target(nullptr) {}

		State *getActiveState();

		void pop();
		void push(std::unique_ptr<State> &state);

		void setGuiTarget(sf::RenderTarget &target);

		//deactivates the statemanager, this will drop all states, and drop utility handles to the console and resource manager
		void drop();
	private:
		State *getValidState(std::vector< std::unique_ptr<State> >::reverse_iterator state);
		std::vector< std::unique_ptr<State> > _states;

		sf::RenderTarget *_target;
	};
}//hades

#endif //HADES_STATEMANAGER_HPP
