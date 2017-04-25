#ifndef HADES_ACTION_HPP
#define HADES_ACTION_HPP

#include <functional>
#include <map>
#include <unordered_set>
#include <tuple>
#include <vector>

#include "SFML/Window/Event.hpp"

#include "Hades/Types.hpp"
#include "Hades/UniqueId.hpp"

namespace hades
{
	//The input system collects all the user input each frame and outputs
	// an array of the current state

	//The system must have a group of actions added to it
	// which must then be bound to interpretors to collect the input

	//this is held by the app object and kept up to date by it.
	//the input state is either collectable by the current state via a function
	// or is passed into the state with it's tick function

	struct Action
	{
		data::UniqueId id;
		int x_axis = 100, y_axis = 100; //joystick movement if joystick; //mouse position if mouse // 100 otherwise
		bool active = false; //always true for mouseposition and axis
	};

	struct InputInterpretor
	{
		data::UniqueId id;
		std::function<std::tuple<bool, Action>(sf::Event, data::UniqueId)> eventCheck;
		std::function<Action(data::UniqueId)> statusCheck;
	};

	class InputSystem
	{
	public:
		void create(data::UniqueId action, bool rebindable, InputInterpretor default);
		void addInterpretor(types::string name, InputInterpretor i);
		void bind(data::UniqueId, types::string);
		void unbind(data::UniqueId, types::string);
		void unbind(data::UniqueId);
		//load bindings config
		//save binding config, //this only works for bindable actions
		void generateState(std::vector<sf::Event> unhandled); //runs all the action test functions, including custom ones
		using action_set = std::unordered_set<Action>;
		action_set getInputState() const;
	private:
		//a map of interpretors to actions, this is used for generateState
		std::multimap<InputInterpretor, data::UniqueId> inputMap;
		//bindable list(lists which actions can be rebound by console or settings interface)
		std::map<data::UniqueId, bool> _bindable;
		std::map<types::string, InputInterpretor> _interpretors;
		std::map<types::string, InputInterpretor> _specialInterpretors;
	};
}

#endif //HADES_ACTION_HPP
