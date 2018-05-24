#ifndef HADES_INPUT_HPP
#define HADES_INPUT_HPP

#include <functional>
#include <map>
#include <set>
#include <tuple>
#include <vector>

#include "SFML/Window/Event.hpp"
#include "SFML/Window/Window.hpp"

#include "Hades/Types.hpp"
#include "hades/uniqueid.hpp"
#include "Hades/vector_math.hpp"

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
		Action(unique_id uid) : id(uid)
		{}

		unique_id id = unique_id::zero;
		hades::types::int32 x_axis = 100, y_axis = 100; //joystick movement if joystick; //mouse position if mouse // 100 otherwise
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
			std::hash<hades::unique_id::type> h;
			return h(key.id.get());
		}
	};
}

namespace hades
{
	using Event = std::tuple<bool, sf::Event>;

	struct InputInterpretor
	{
		unique_id id = unique_id::zero;
		using event_function = std::function<Action(bool handled, const sf::Event&)>;
		event_function eventCheck;
		using function = std::function<Action(void)>;
		function statusCheck;
		using event_match_function = std::function<bool(const sf::Event&)>;
		event_match_function is_match;
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
		void create(unique_id action, bool rebindable);
		void create(unique_id action, bool rebindable, types::string defaultBinding);
		//adds a new input interpretor
		void addInterpretor(types::string name, InputInterpretor::event_match_function m, InputInterpretor::event_function e);
		void addInterpretor(types::string name, InputInterpretor::function f);
		void addInterpretor(types::string name, InputInterpretor::event_match_function m, InputInterpretor::event_function e, InputInterpretor::function f);
		//binds an action to an interpretor
		bool bind(unique_id, types::string);
		//unbinds a specific interpretor from an action
		void unbind(unique_id, types::string);
		//unbinds all interpretors from an action
		void unbind(unique_id);
		//load bindings config
		//save binding config, //this only works for bindable actions
		void generateState(const std::vector<Event>&); //runs all the action test functions, including custom ones
		using action_set = std::set<Action>;
		action_set getInputState() const;
	private:

		template<sf::Keyboard::Key k>
		InputInterpretor Keyboard();

		template<sf::Mouse::Button b>
		InputInterpretor MouseButton(const sf::Window &window);

		//a map of interpretors to actions, this is used for generateStat
		using input_map = std::multimap<InputInterpretor, unique_id>;
		input_map _inputMap;
		//bindable list(lists which actions can be rebound by console or settings interface)
		std::map<unique_id, bool> _bindable;
		std::map<types::string, InputInterpretor> _interpretors;
		std::map<types::string, InputInterpretor> _specialInterpretors;
		action_set _previousState;
	};
}

#endif //HADES_INPUT_HPP
