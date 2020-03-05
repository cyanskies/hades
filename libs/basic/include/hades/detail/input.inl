#include "hades/input.hpp"

namespace hades
{
	template<typename Event>
	void input_event_system_t<Event>::add_interpreter(std::string_view name, typename event_interpreter::event_match_function m, typename event_interpreter::event_function e)
	{
		input_interpreter::interpreter_id id{};
		_add_interpreter_name(name, id);

		event_interpreter in{};
		in.id = id;
		assert(e && m);
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
		assert(e && m);
		en.event_check = e;
		en.is_match = m;

		_event_interpreters.insert({ en, {} });

		if (f)
		{
			input_interpreter in{ f };
			in.id = id;
			_interpreters.insert({ in,{} });
		}
	}

	namespace detail
	{
		template<typename Event>
		static action check_events(const typename input_event_system_t<Event>::event_interpreter& interpreter,
			const std::vector<typename input_event_system_t<Event>::checked_event>& events, action a)
		{
			for (const auto& e : events)
			{
				const auto& this_event = std::get<Event>(e);
				//check every event against this interpreter
				if (std::invoke(interpreter.is_match, this_event))
					a = std::invoke(interpreter.event_check, std::get<bool>(e), this_event, a);
			}

			return a;
		}
	}

	template<typename Event>
	void input_event_system_t<Event>::generate_state(const std::vector<typename input_event_system_t<Event>::checked_event> &events)
	{
		input_system::generate_state();

		for (auto& [i, a] : _event_interpreters)
			a = detail::check_events<Event>(i, events, a);
	}
	
	template<typename Event>
	inline typename input_event_system_t<Event>::action_set input_event_system_t<Event>::input_state() const
	{
		auto out = action_set{};
		auto iter = std::begin(_action_input);
		const auto end = std::end(_action_input);

		while (iter != end)
		{
			auto [begin, rng_end] = _action_input.equal_range(iter->first);

			auto a = action{};
			while (begin != rng_end)
			{
				const auto interpreter = _interpreters.find({ begin->second });
				if(interpreter != std::end(_interpreters))
					a.merge(interpreter->second);
				const auto event = _event_interpreters.find({ begin->second });
				if (event != std::end(_event_interpreters))
					a.merge(event->second);
				++begin;
			}

			a.id = iter->first;
			out.emplace(a);
			iter = rng_end;
		}

		return out;
	}
}
