#ifndef HADES_INPUT_HPP
#define HADES_INPUT_HPP

#include <functional>
#include <map>
#include <set>
#include <tuple>
#include <unordered_map>

#include "hades/string.hpp"
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

	struct action
	{
		constexpr action() noexcept = default;
		explicit constexpr action(unique_id uid) noexcept : id{ uid }
		{}
		constexpr action(unique_id uid, int32 x) noexcept : id{ uid }, x_axis{ x }
		{}
		constexpr action(unique_id uid, int32 x, int32 y) noexcept : id{ uid }, x_axis{ x }, y_axis{ y }
		{}

		explicit constexpr action(bool active) noexcept : active(active)
		{}
		constexpr action(unique_id i, int32 x, int32 y, int32 u, int32 v) noexcept :
			id{ i }, x_axis{ x }, y_axis{ y }, u_axis{ u }, v_axis{v}
		{}

		void merge(const action &other) noexcept;

		unique_id id = unique_id::zero;
		types::int32 x_axis = 0, y_axis = 0, //joystick movement if joystick; //mouse position if mouse // 0 otherwise
			u_axis = 0, v_axis = 0;
		bool active = false; //always true for mouseposition and axis
	};

	inline constexpr  bool operator<(const action &lhs, const action &rhs)
	{
		return lhs.id < rhs.id;
	}

	inline constexpr bool operator==(const action &lhs, const action &rhs)
	{
		return lhs.id == rhs.id;
	}

	struct input_interpreter
	{
		using interpreter_id = unique_id;
		using function = std::function<action(action)>;

		input_interpreter() noexcept = default;
		input_interpreter(function func) noexcept : input_check(func) {}
		input_interpreter(interpreter_id id) noexcept : id{ id } {}

		interpreter_id id = interpreter_id::zero;
		//a function that generates an action by reading the current state of the input device
		function input_check;
	};

	inline constexpr bool operator<(const input_interpreter& lhs, const input_interpreter& rhs)
	{
		return lhs.id < rhs.id;
	}

	inline constexpr bool operator==(const input_interpreter& lhs, const input_interpreter& rhs)
	{
		return lhs.id == rhs.id;
	}

	template<typename Event>
	struct input_event_interpreter
	{
		using event = Event;
		using interpreter_id = input_interpreter::interpreter_id;

		constexpr input_event_interpreter() = default;
		constexpr input_event_interpreter(interpreter_id id) : id(id) {}

		interpreter_id id = interpreter_id::zero;
		using event_function = std::function<action(bool, const event&, action)>;
		event_function event_check{};
		using event_match_function = bool(*)(const event&);
		event_match_function is_match{};
	};

	template<typename Event>
	inline constexpr bool operator<(const input_event_interpreter<Event>& lhs, const input_event_interpreter<Event>& rhs)
	{
		return lhs.id < rhs.id;
	}

	template<typename Event>
	inline constexpr bool operator==(const input_event_interpreter<Event>& lhs, const input_event_interpreter<Event>& rhs)
	{
		return lhs.id == rhs.id;
	}
}

template <>
struct std::hash<hades::action>
{
	size_t operator()(const hades::action& key) const noexcept
	{
		std::hash<hades::unique_id::type> h;
		return h(key.id.get());
	}
};

template <>
struct std::hash<hades::input_interpreter>
{
	size_t operator()(const hades::input_interpreter& key) const noexcept
	{
		std::hash<hades::input_interpreter::interpreter_id::type> h;
		return h(key.id.get());
	}
};

template <typename Event>
struct std::hash<hades::input_event_interpreter<Event>>
{
	size_t operator()(const hades::input_event_interpreter<Event>& key) const noexcept
	{
		std::hash<hades::input_event_interpreter<Event>::interpreter_id::type> h;
		return h(key.id.get());
	}
};

namespace hades
{
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

		using interpreter_set = std::unordered_map<input_interpreter, action>;
		interpreter_set _interpreters;
		void _add_interpreter_name(std::string_view name, input_interpreter::interpreter_id id);

	private:		
		//bindable list(lists which actions can be rebound by console or settings interface)
		std::map<unique_id, bool> _bindable;
		//key is usually a single character, so normal string comparison is preffered
		using interpreter_name_map = std::map<string, input_interpreter::interpreter_id>;
		interpreter_name_map _interpreter_names;
	};

	

	template<typename Event>
	class input_event_system_t final : public input_system
	{
	public:
		using event = Event;
		//using checked_event = std::tuple<bool, event>;
		using event_interpreter = input_event_interpreter<event>;
		using event_interpreter_set = std::unordered_map<event_interpreter, action>;

		struct checked_event
		{
			bool handled;
			event event;
		};

		//adds a new input interpretor
		void add_interpreter(std::string_view name, typename event_interpreter::event_match_function m, typename event_interpreter::event_function e);
		void add_interpreter(std::string_view name, typename event_interpreter::event_match_function m, typename event_interpreter::event_function e, input_interpreter::function f);

		void generate_state(const std::vector<checked_event>&); //runs all the action test functions, including custom ones
		action_set input_state() const;
	private:
		event_interpreter_set _event_interpreters;
	};
}

#include "hades/detail/input.inl"

#endif //HADES_INPUT_HPP
