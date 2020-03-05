#include "hades/input.hpp"

#include <algorithm>
#include <cassert>

#include "hades/uniqueid.hpp"
#include "hades/utility.hpp"

namespace hades
{
	void action::merge(const action &other)
	{
		if (!active && other.active)
			*this = other;
		//else if (active && other.active)
		//{
		//	// this is appropriate for non-joystick input?
		//	x_axis = std::max(x_axis, other.x_axis);
		//	y_axis = std::max(y_axis, other.y_axis);

		//	////TODO: should we do this?
		//	// No, we shouldn't, it bugs the mouse input
		//	//x_axis = std::clamp<decltype(x_axis)>(x_axis, 0, 100);
		//	//y_axis = std::clamp<decltype(y_axis)>(y_axis, 0, 100);
		//}
	}

	input_interpreter::input_interpreter(function func) : input_check(func)
	{}

	input_interpreter::input_interpreter(interpreter_id id) : id(id)
	{}

	void input_system::create(input_system::action_id action, bool rebindable)
	{
		_bindable.insert({ action, rebindable });
	}

	void input_system::create(input_system::action_id action, bool rebindable, std::string_view default_binding)
	{
		const auto default_interpreter = _interpreter_names.find(to_string(default_binding));
		assert(default_interpreter != std::end(_interpreter_names));
		//TODO: throw if interpreter not found
		_action_input.insert({ action, default_interpreter->second});
		_bindable.insert({ action, rebindable });
	}

	void input_system::add_interpreter(std::string_view name, input_interpreter::function f)
	{
		input_interpreter::interpreter_id id{};
		_add_interpreter_name(name, id);

		input_interpreter in{ f };
		in.id = id;

		_interpreters.insert({ in, {} });
	}

	bool input_system::bind(input_system::action_id action, std::string_view interpretor)
	{
		if (_bindable.find(action) == _bindable.end())
			return false;

		auto inter = _interpreter_names.find(to_string(interpretor));
		if (inter == std::end(_interpreter_names))
			return false;

		_action_input.insert({ action, inter->second });
		return true;
	}

	void input_system::unbind(input_system::action_id action, std::string_view input)
	{
		auto inter = _interpreter_names.find(to_string(input));
		if (inter == std::end(_interpreter_names))
			return;

		//check all the bindings for 'action'
		auto [iter, end] = _action_input.equal_range(action);

		while (iter != end)
		{
			//if the binding is for the named input
			if (iter->second == iter->second)
				iter = _action_input.erase(iter);
			else
				++iter;
		}
	}

	void input_system::unbind(input_system::action_id action)
	{
		const auto[begin, end] = _action_input.equal_range(action);
		_action_input.erase(begin, end);
	}

	void input_system::generate_state()
	{
		for (auto& [i, a] : _interpreters)
			a = std::invoke(i.input_check, a);
	}

	typename input_system::action_set input_system::input_state() const
	{
		auto out = action_set{};
		auto iter = std::begin(_action_input);
		const auto end = std::end(_action_input);

		const auto get_action = [&](input_interpreter::interpreter_id i)->action {
			return _interpreters.find({ i })->second;
		};

		while (iter != end)
		{
			auto [begin, rng_end] = _action_input.equal_range(iter->first);

			//get the action generated but each input
			auto a = get_action(begin->second);
			while (++begin != rng_end)
				a.merge(get_action(begin->second));

			a.id = iter->first;
			out.emplace(a);
			iter = rng_end;
		}

		return out;
	}

	void input_system::_add_interpreter_name(std::string_view name, input_interpreter::interpreter_id id)
	{
		_interpreter_names.insert_or_assign(to_string(name), id);
	}
}
