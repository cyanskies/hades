#include "hades/input.hpp"

namespace hades
{
	template<typename Event>
	void input_event_system_t<Event>::add_interpreter(std::string_view name,
		event_interpreter::event_match_function m, event_interpreter::event_function e)
	{
		const auto id = make_unique_id();
		_add_interpreter_name(name, id);

		assert(e && m);
		event_interpreter in{ id, std::move(e), std::move(m) };
		_event_interpreters.insert({ std::move(in), {} });
	}

	template<typename Event>
	void input_event_system_t<Event>::add_interpreter(std::string_view name, 
		event_interpreter::event_match_function m, event_interpreter::event_function e, input_interpreter::function f)
	{
		const auto id = make_unique_id();
		_add_interpreter_name(name, id);

		assert(e && m);
		event_interpreter in{ id, std::move(e), std::move(m) };
		_event_interpreters.insert({ std::move(in), {} });

		if (f)
		{
			input_interpreter in{ id, std::move(f) };
			_interpreters.insert({ std::move(in), {} });
		}
	}

	namespace detail
	{
		template<typename Event>
		static action check_event(const typename input_event_system_t<Event>::event_interpreter& interpreter,
			const typename input_event_system_t<Event>::event e, action a)
		{ 
			if (std::invoke(interpreter.is_match, e))
				return std::invoke(interpreter.event_check, e, a);
			return a;
		}
	}

	template<typename Event>
	void input_event_system_t<Event>::insert_event(const typename input_event_system_t<Event>::event e)
	{
		for (auto& [i, a] : _event_interpreters)
			a = detail::check_event<Event>(i, e, a);
	}
	
	template<typename Event>
	inline const typename input_event_system_t<Event>::action_set& input_event_system_t<Event>::input_state()
	{
		auto& out = _action_set;
		out.clear();
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
