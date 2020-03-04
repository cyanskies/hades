#include "hades/input.hpp"

namespace hades
{
	constexpr time_duration double_tap_time = time_cast<time_duration>(seconds_float{ 0.30f });

	template<bool DoubleTap, bool Holdable>
	constexpr inline void update_action_state(const action& a, const time_point& t, action_state<DoubleTap, Holdable>& s) noexcept
	{
		//check each combination of action
		//action is down / not down
		//action was down / wans't down last frame

		//test for action being activated or double tapped
		if (a.active && !s.pressed)
		{
			if constexpr (DoubleTap)
			{
				assert(t > s.last_active);
				const time_duration active_duration = t - s.last_active;
				
				//mouse has been clicked previously
				if (active_duration < double_tap_time)
				{
					s.double_tapped = true;
					s.last_active = {};
				}
				else
				{
					s.last_active = t;
					s.pressed = true;
				}
			}
			else
				s.pressed = true;
		}
		if (a.active && s.pressed)
		{
			s.pressed = false;
			if constexpr (Holdable)
			{
				if (t != s.last_active)
					s.held = true;
			}
		}
		else if(!a.active)
		{
			s.pressed = false;
			if constexpr (Holdable)
				s.held = false;
			if constexpr (DoubleTap)
				s.double_tapped = false;
		}
	}

	template<bool DoubleTap, bool Holdable>
	constexpr bool action_pressed(const action_state<DoubleTap, Holdable>& state) noexcept
	{
		return state.pressed;
	}

	template<bool DoubleTap, bool Holdable>
	constexpr bool action_down(const action_state<DoubleTap, Holdable>& state) noexcept 
	{
		if constexpr (Holdable)
		{
			return state.held || state.pressed;
		}
		else
			return state.pressed;
	}

	template<bool DoubleTap, bool Holdable>
	constexpr bool action_double_tap(const action_state<DoubleTap, Holdable>& state) noexcept
	{
		static_assert(DoubleTap, "Double tap is disabled for this mouse state");
		return false;
	}

	template<bool Holdable>
	constexpr bool action_double_tap(const action_state<true, Holdable>& state) noexcept
	{
		return state.double_tap;
	}

	template<bool DoubleTap, bool Holdable>
	constexpr bool action_held(const action_state<DoubleTap, Holdable>& state) noexcept
	{
		static_assert(Holdable, "holding is disabled for this mouse state");
		return false;
	}

	template<bool DoubleTap>
	constexpr bool action_held(const action_state<DoubleTap, true>& state) noexcept
	{
		return state.held;
	}

	template<bool DoubleTap, bool Holdable>
	constexpr time_point action_hold_start_time(const action_state<DoubleTap, Holdable>& state) noexcept
	{
		static_assert(Holdable, "holding is disabled for this mouse state");
		return false;
	}

	template<bool DoubleTap>
	constexpr bool action_hold_start_time(const action_state<DoubleTap, true>& state) noexcept
	{
		return state.last_active;
	}

	template<typename Event>
	void input_event_system_t<Event>::add_interpreter(std::string_view name, typename event_interpreter::event_match_function m, typename event_interpreter::event_function e)
	{
		input_interpreter::interpreter_id id{};
		_add_interpreter_name(name, id);

		event_interpreter in{};
		in.id = id;
		in.event_check = e;
		in.is_match = m;

		_event_interpreters.insert(in);
	}

	template<typename Event>
	void input_event_system_t<Event>::add_interpreter(std::string_view name, typename event_interpreter::event_match_function m, typename event_interpreter::event_function e, input_interpreter::function f)
	{
		input_interpreter::interpreter_id id{};
		_add_interpreter_name(name, id);

		event_interpreter en{};
		en.id = id;
		en.event_check = e;
		en.is_match = m;

		_event_interpreters.insert(en);

		if (f)
		{
			input_interpreter in{ f };
			in.id = id;
			_interpreters.insert(in);
		}
	}

	template<typename Event>
	std::tuple<action, bool> check_events(const typename input_event_system_t<Event>::event_interpreter &interpreter,
		const std::vector<typename input_event_system_t<Event>::checked_event> &events)
	{
		action a{};
		bool handled = false;

		for (const auto &e : events)
		{
			const auto &this_event = std::get<Event>(e);
			//check every event against this interpreter
			if (interpreter.is_match
				&& interpreter.is_match(this_event)
				&& interpreter.event_check)
			{
				a.merge(interpreter.event_check(std::get<bool>(e), this_event));
				handled = true;
			}
		}

		return { a, handled };
	}

	template<typename Event>
	void input_event_system_t<Event>::generate_state(const std::vector<typename input_event_system_t<Event>::checked_event> &events)
	{
		input_system::action_set actionset;

		auto iter = std::begin(_action_input);
		const auto range_end = std::end(_action_input);
		//for each action
		while (iter != range_end)
		{
			//get the list of interpreters that are rigged to that action
			auto[begin, end] = _action_input.equal_range(iter->first);

			action a{};
			bool handled = false;
			while (begin != end)
			{
				//for each interpreter
				const auto interpreter = _event_interpreters.find({ (begin)->second });
				auto interpretor_handled = false;
				if (interpreter != std::end(_event_interpreters))
				{
					const std::tuple<action, bool> ret = check_events<Event>(*interpreter, events);
					a.merge(std::get<action>(ret));
					const bool event_handled = std::get<bool>(ret);
					interpretor_handled |= event_handled;
				}

				//if none of the events resolved the interpreter
				//then run the real-time input check if present
				if (!interpretor_handled)
				{
					const auto interpreter = _interpreters.find({ (begin)->second });
					if (interpreter != std::end(_interpreters) && interpreter->input_check)
					{
						a.merge(interpreter->input_check());
						interpretor_handled = true;
					}	
				}

				handled |= interpretor_handled;
				++begin;
			}

			//if we can't get up to date info on the input, then reuse the previous entry.
			/*if (!handled)
			{
				const auto prev = _previous_state.find({ iter->first });
				if (prev != std::end(_previous_state))
					a = *prev;
			}*/

			a.id = iter->first;
			actionset.emplace(a);

			iter = end;
		}

		actionset.swap(_previous_state);
	}
}
