#ifndef HADES_INPUT_HPP
#define HADES_INPUT_HPP

#include <functional>
#include <map>
#include <set>
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
		Action() = default;
		Action(data::UniqueId uid) : id(uid)
		{}

		data::UniqueId id = data::UniqueId::Zero;
		int x_axis = 100, y_axis = 100; //joystick movement if joystick; //mouse position if mouse // 100 otherwise
		bool active = false; //always true for mouseposition and axis
	};

	inline bool operator<(const Action &lhs, const Action &rhs)
	{
		return lhs.id < rhs.id;
	}

	inline bool operator==(const Action &lhs, const Action &rhs)
	{
		return lhs.id == rhs.id;
	}
}

namespace std {
	template <> 
	struct hash<hades::Action>
	{
		size_t operator()(const hades::Action& key) const
		{
			std::hash<hades::data::UniqueId::type> h;
			return h(key.id.get());
		}
	};
}

namespace hades
{
	struct InputInterpretor
	{
		data::UniqueId id = data::UniqueId::Zero;
		using event_function = std::function<std::tuple<bool, Action>(const sf::Event&, data::UniqueId)>;
		event_function eventCheck;
		using function = std::function<Action(data::UniqueId)>;
		function statusCheck;
	};

	inline bool operator<(const InputInterpretor &lhs, const InputInterpretor &rhs)
	{
		return lhs.id < rhs.id;
	}

	class InputSystem
	{
	public:
		InputSystem(const sf::Window&);

		//creates an action and sets it's bindable status
		void create(data::UniqueId action, bool rebindable);
		void create(data::UniqueId action, bool rebindable, types::string defaultBinding);
		//adds a new input interpretor
		void addInterpretor(types::string name, InputInterpretor::event_function e, InputInterpretor::function f);
		//binds an action to an interpretor
		bool bind(data::UniqueId, types::string);
		//unbinds a specific interpretor from an action
		void unbind(data::UniqueId, types::string);
		//unbinds all interpretors from an action
		void unbind(data::UniqueId);
		//load bindings config
		//save binding config, //this only works for bindable actions
		void generateState(std::vector<sf::Event> unhandled); //runs all the action test functions, including custom ones
		using action_set = std::set<Action>;
		action_set getInputState() const;
	private:
		//a map of interpretors to actions, this is used for generateStat
		using input_map = std::multimap<InputInterpretor, data::UniqueId>;
		input_map _inputMap;
		//bindable list(lists which actions can be rebound by console or settings interface)
		std::map<data::UniqueId, bool> _bindable;
		std::map<types::string, InputInterpretor> _interpretors;
		std::map<types::string, InputInterpretor> _specialInterpretors;
		std::vector<Action> _inputState;
	};
}

#endif //HADES_INPUT_HPP
