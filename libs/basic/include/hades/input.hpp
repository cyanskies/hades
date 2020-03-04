#ifndef HADES_INPUT_HPP
#define HADES_INPUT_HPP

#include <functional>
#include <map>
#include <set>
#include <tuple>
#include <vector>

#include "hades/time.hpp"
#include "hades/types.hpp"
#include "hades/uniqueid.hpp"

namespace hades
{
	//The input system collects all the user input each frame and outputs
	// an array of the current state

	//The system must have a group of actions added to it
	// which must then be bound to interpretors to collect the input

	//this is held by the app object and kept up to date by it.
	//the input state is either collectable by the current state via a function
	// or is passed into the state with it's tick function

	//TODO: redo input logic, current system doesn't handle muiltible keys bound to the same action gracefully
	// also doesn't have the best reliability in regards to keys being left active and so on.
	// store activity state with keys first, then record activation based on this

	struct action
	{
		action() = default;
		action(unique_id uid) : id(uid)
		{}
		action(bool active) : active(active)
		{}

		void merge(const action &other);

		unique_id id = unique_id::zero;
		types::int32 x_axis = 0, y_axis = 0, //joystick movement if joystick; //mouse position if mouse // 0 otherwise
			u_axis = 0, v_axis = 0;
		bool active = false; //always true for mouseposition and axis
	};

	inline bool operator<(const action &lhs, const action &rhs)
	{
		return lhs.id < rhs.id;
	}

	inline bool operator==(const action &lhs, const action &rhs)
	{
		return lhs.id == rhs.id;
	}
}

namespace std {
	template <> 
	struct hash<hades::action>
	{
		size_t operator()(const hades::action& key) const
		{
			std::hash<hades::unique_id::type> h;
			return h(key.id.get());
		}
	};
}

namespace hades
{
	namespace detail
	{
		struct action_extra
		{
			time_point last_active = {};
		};

		struct action_double
		{
			bool double_tapped = false;
		};

		struct action_held
		{
			bool held = false;
		};

		struct action_empty {};
	}

	//can be used to track individual presses.
	// double taps
	// and holding the button down.
	template<bool DoubleTap = true, bool Holdable = true>
	struct action_state : public std::conditional_t<DoubleTap || Holdable, detail::action_extra, detail::action_empty>,
		public std::conditional_t<DoubleTap, detail::action_double, detail::action_empty>,
		public std::conditional_t<Holdable, detail::action_held, detail::action_empty>
	{
		bool pressed = false;
	};

	//functions for testing the state
	template<bool DoubleTap, bool Holdable>
	constexpr void update_action_state(const action& a, const time_point& time, action_state<DoubleTap, Holdable>& state) noexcept;
	template<bool DoubleTap, bool Holdable>
	constexpr bool action_pressed(const action_state<DoubleTap, Holdable>& state) noexcept; // returns true if the button was just pressed
	template<bool DoubleTap, bool Holdable>
	constexpr bool action_down(const action_state<DoubleTap, Holdable>& state) noexcept; // returns true as long as the button is down; pressed || held
	template<bool DoubleTap, bool Holdable>
	constexpr bool action_double_tap(const action_state<DoubleTap, Holdable>& state) noexcept; // returns true if the button has been double tapped
	template<bool DoubleTap, bool Holdable>
	constexpr bool action_held(const action_state<DoubleTap, Holdable>& state) noexcept; // returns true if the button has been held down
	template<bool DoubleTap, bool Holdable>
	constexpr time_point action_hold_start_time(const action_state<DoubleTap, Holdable>& state) noexcept;

	struct input_interpreter
	{
		using interpreter_id = unique_id;
		using function = std::function<action(void)>;

		input_interpreter() = default;
		input_interpreter(function func);
		input_interpreter(interpreter_id);

		interpreter_id id = interpreter_id::zero;
		//a function that generates an action by reading the current state of the input device
		function input_check;
	};

	inline bool operator<(const input_interpreter &lhs, const input_interpreter &rhs)
	{
		return lhs.id < rhs.id;
	}

	class input_system
	{
	public:
		using action_id = unique_id;

		void create(action_id action, bool rebindable);
		void create(action_id action, bool rebindable, std::string_view default_binding);

		//adds a new input interpretor
		void add_interpreter(std::string_view name, input_interpreter::function f);

		//binds an action to an interpretor
		bool bind(action_id, std::string_view);
		//unbinds a specific interpretor from an action
		void unbind(action_id, std::string_view);
		//unbinds all interpretors from an action
		void unbind(action_id); 
		//load bindings config
		//save binding config, //this only works for bindable actions
		void generate_state(); //runs all the action test functions, including custom ones
		using action_set = std::set<action>;
		action_set input_state() const;

	protected:
		//a map of interpretors to actions, this is used for generateStat
		using input_map = std::multimap<action_id, input_interpreter::interpreter_id>;
		input_map _action_input;

		using interpreter_set = std::set<input_interpreter>;
		interpreter_set _interpreters;

		action_set _previous_state;

		void _add_interpreter_name(std::string_view name, input_interpreter::interpreter_id id);

	private:		
		//bindable list(lists which actions can be rebound by console or settings interface)
		std::map<unique_id, bool> _bindable;
		using interpreter_name_map = std::map<types::string, input_interpreter::interpreter_id>;
		interpreter_name_map _interpreter_names;
	};

	template<typename Event>
	struct input_event_interpreter
	{
		using event = Event;
		using interpreter_id = input_interpreter::interpreter_id;

		input_event_interpreter() = default;
		input_event_interpreter(interpreter_id id) : id(id) {}

		interpreter_id id = interpreter_id::zero;
		using event_function = std::function<action(bool handled, const event&)>;
		event_function event_check;
		using event_match_function = std::function<bool(const event&)>;
		event_match_function is_match;
	};

	template<typename Event>
	inline bool operator<(const input_event_interpreter<Event> &lhs, const input_event_interpreter<Event> &rhs)
	{
		return lhs.id < rhs.id;
	}

	template<typename Event>
	class input_event_system_t final : public input_system
	{
	public:
		using event = Event;
		using checked_event = std::tuple<bool, event>;
		using event_interpreter = input_event_interpreter<event>;
		using event_interpreter_set = std::set<event_interpreter>;

		//adds a new input interpretor
		void add_interpreter(std::string_view name, typename event_interpreter::event_match_function m, typename event_interpreter::event_function e);
		void add_interpreter(std::string_view name, typename event_interpreter::event_match_function m, typename event_interpreter::event_function e, input_interpreter::function f);

		void generate_state(const std::vector<checked_event>&); //runs all the action test functions, including custom ones
	private:
		event_interpreter_set _event_interpreters;
	};
}

#include "hades/detail/input.inl"

#endif //HADES_INPUT_HPP
