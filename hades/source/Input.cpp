#include "Hades/Input.hpp"

#include <cassert>

#include "SFML/Window/Keyboard.hpp"
#include "SFML/Window/Mouse.hpp"
#include "SFML/Window/Touch.hpp"
#include "SFML/Window/Window.hpp"

#include "Hades/App.hpp"
#include "Hades/UniqueId.hpp"
#include "Hades/vector_math.hpp"

namespace hades
{
	template<sf::Keyboard::Key k>
	InputInterpretor InputSystem::Keyboard()
	{
		InputInterpretor i;

		const auto &state = _previousState;
		i.eventCheck = [&state](bool handled, const sf::Event &e, data::UniqueId id) {
			Action a;
			a.id = id;

			if (!handled && e.type == sf::Event::KeyPressed && e.key.code == k)
				a.active = true;
			else if (e.type == sf::Event::KeyReleased && e.key.code == k)
				a.active = false;
			else
				return std::make_tuple(false, a);

			return std::make_tuple(true, a);
		};

		return i;
	}

	std::tuple<types::int32, types::int32, bool> MousePos(const sf::Window &window)
	{
		auto mpos = sf::Mouse::getPosition(window);
		auto size = window.getSize();
		auto inside_window = sf::IntRect(0, 0, size.x, size.y).contains(mpos);

		auto [mx, my] = vector_clamp(mpos.x, mpos.y,
			0, 0,
			static_cast<types::int32>(size.x),
			static_cast<types::int32>(size.y));

		return std::make_tuple(mx, my, inside_window);
	}

	template<sf::Mouse::Button b>
	InputInterpretor InputSystem::MouseButton(const sf::Window &window)
	{
		InputInterpretor i;

		const auto &state = _previousState;
		i.eventCheck = [&window, &state](bool handled, const sf::Event &e, data::UniqueId id) {
			Action a;
			a.id = id;

			if (!handled && e.type == sf::Event::MouseButtonPressed && e.mouseButton.button == b)
			{
				a.active = true;
				std::tie(a.x_axis, a.y_axis) = vector_clamp(e.mouseButton.x, e.mouseButton.y,
					0, 0,
					static_cast<types::int32>(window.getSize().x),
					static_cast<types::int32>(window.getSize().y));
			}
			else if (e.type == sf::Event::MouseButtonReleased && e.mouseButton.button == b)
				a.active = false;
			else
				return std::make_tuple(false, a);

			return std::make_tuple(true, a);
		};

		return i;
	}

	InputSystem::InputSystem(const sf::Window &window)
	{
		//bind all the basic input types
		auto &i = _interpretors;
		//=====keyboard keys=====
		//alpha
		i.insert({ "a", Keyboard<sf::Keyboard::A>() });
		i.insert({ "b", Keyboard<sf::Keyboard::B>() });
		i.insert({ "c", Keyboard<sf::Keyboard::C>() });
		i.insert({ "d", Keyboard<sf::Keyboard::D>() });
		i.insert({ "e", Keyboard<sf::Keyboard::E>() });
		i.insert({ "f", Keyboard<sf::Keyboard::F>() });
		i.insert({ "g", Keyboard<sf::Keyboard::G>() });
		i.insert({ "h", Keyboard<sf::Keyboard::H>() });
		i.insert({ "i", Keyboard<sf::Keyboard::I>() });
		i.insert({ "j", Keyboard<sf::Keyboard::J>() });
		i.insert({ "k", Keyboard<sf::Keyboard::K>() });
		i.insert({ "l", Keyboard<sf::Keyboard::L>() });
		i.insert({ "m", Keyboard<sf::Keyboard::M>() });
		i.insert({ "n", Keyboard<sf::Keyboard::N>() });
		i.insert({ "o", Keyboard<sf::Keyboard::O>() });
		i.insert({ "p", Keyboard<sf::Keyboard::P>() });
		i.insert({ "q", Keyboard<sf::Keyboard::Q>() });
		i.insert({ "r", Keyboard<sf::Keyboard::R>() });
		i.insert({ "s", Keyboard<sf::Keyboard::S>() });
		i.insert({ "t", Keyboard<sf::Keyboard::T>() });
		i.insert({ "u", Keyboard<sf::Keyboard::U>() });
		i.insert({ "v", Keyboard<sf::Keyboard::V>() });
		i.insert({ "w", Keyboard<sf::Keyboard::W>() });
		i.insert({ "x", Keyboard<sf::Keyboard::X>() });
		i.insert({ "y", Keyboard<sf::Keyboard::Y>() });
		i.insert({ "z", Keyboard<sf::Keyboard::Z>() });
		//numeric
		i.insert({ "0", Keyboard<sf::Keyboard::Num0>() });
		i.insert({ "1", Keyboard<sf::Keyboard::Num1>() });
		i.insert({ "2", Keyboard<sf::Keyboard::Num2>() });
		i.insert({ "3", Keyboard<sf::Keyboard::Num3>() });
		i.insert({ "4", Keyboard<sf::Keyboard::Num4>() });
		i.insert({ "5", Keyboard<sf::Keyboard::Num5>() });
		i.insert({ "6", Keyboard<sf::Keyboard::Num6>() });
		i.insert({ "7", Keyboard<sf::Keyboard::Num7>() });
		i.insert({ "8", Keyboard<sf::Keyboard::Num8>() });
		i.insert({ "9", Keyboard<sf::Keyboard::Num9>() });
		//control
		i.insert({ "escape", Keyboard<sf::Keyboard::Escape>() });
		i.insert({ "lcontrol", Keyboard<sf::Keyboard::LControl>() });
		i.insert({ "lshift", Keyboard<sf::Keyboard::RControl>() });
		i.insert({ "lalt", Keyboard<sf::Keyboard::LAlt>() });
		//i.insert({ "lsystem", Keyboard<sf::Keyboard::Num3>() }); //don't rebind system key
		i.insert({ "rcontrol", Keyboard<sf::Keyboard::RControl>() });
		i.insert({ "rshift", Keyboard<sf::Keyboard::RShift>() });
		i.insert({ "ralt", Keyboard<sf::Keyboard::RAlt>() });
		//i.insert({ "rsystem", Keyboard<sf::Keyboard::Num7>() });
		i.insert({ "menu", Keyboard<sf::Keyboard::Menu>() });
		//special
		i.insert({ "lbracket", Keyboard<sf::Keyboard::LBracket>() });
		i.insert({ "rbracket", Keyboard<sf::Keyboard::RBracket>() });
		i.insert({ "semicolon", Keyboard<sf::Keyboard::SemiColon>() });
		i.insert({ "comma", Keyboard<sf::Keyboard::Comma>() });
		i.insert({ "period", Keyboard<sf::Keyboard::Period>() });
		i.insert({ "quote", Keyboard<sf::Keyboard::Quote>() });
		i.insert({ "slash", Keyboard<sf::Keyboard::Slash>() });
		i.insert({ "backslash", Keyboard<sf::Keyboard::BackSlash>() });
		//i.insert({ "tilde", Keyboard<sf::Keyboard::Num9>() });
		i.insert({ "equal", Keyboard<sf::Keyboard::Equal>() });
		i.insert({ "dash", Keyboard<sf::Keyboard::Dash>() });
		i.insert({ "space", Keyboard<sf::Keyboard::Space>() });
		i.insert({ "return", Keyboard<sf::Keyboard::Return>() });
		i.insert({ "backspace", Keyboard<sf::Keyboard::BackSpace>() });
		i.insert({ "tab", Keyboard<sf::Keyboard::Tab>() });
		i.insert({ "pageup", Keyboard<sf::Keyboard::PageUp>() });
		i.insert({ "pagedown", Keyboard<sf::Keyboard::PageDown>() });
		i.insert({ "end", Keyboard<sf::Keyboard::End>() });
		i.insert({ "home", Keyboard<sf::Keyboard::Home>() });
		i.insert({ "insert", Keyboard<sf::Keyboard::Insert>() });
		i.insert({ "delete", Keyboard<sf::Keyboard::Delete>() });
		i.insert({ "add", Keyboard<sf::Keyboard::Add>() });
		//numpad
		i.insert({ "subtract", Keyboard<sf::Keyboard::Subtract>() });
		i.insert({ "multiply", Keyboard<sf::Keyboard::Multiply>() });
		i.insert({ "divide", Keyboard<sf::Keyboard::Divide>() });
		i.insert({ "left", Keyboard<sf::Keyboard::Left>() });
		i.insert({ "right", Keyboard<sf::Keyboard::Right>() });
		i.insert({ "up", Keyboard<sf::Keyboard::Up>() });
		i.insert({ "down", Keyboard<sf::Keyboard::Down>() });
		i.insert({ "num0", Keyboard<sf::Keyboard::Numpad0>() });
		i.insert({ "num1", Keyboard<sf::Keyboard::Numpad1>() });
		i.insert({ "num2", Keyboard<sf::Keyboard::Numpad2>() });
		i.insert({ "num3", Keyboard<sf::Keyboard::Numpad3>() });
		i.insert({ "num4", Keyboard<sf::Keyboard::Numpad4>() });
		i.insert({ "num5", Keyboard<sf::Keyboard::Numpad5>() });
		i.insert({ "num6", Keyboard<sf::Keyboard::Numpad6>() });
		i.insert({ "num7", Keyboard<sf::Keyboard::Numpad7>() });
		i.insert({ "num8", Keyboard<sf::Keyboard::Numpad8>() });
		i.insert({ "num9", Keyboard<sf::Keyboard::Numpad9>() });
		i.insert({ "f1", Keyboard<sf::Keyboard::F1>() });
		i.insert({ "f2", Keyboard<sf::Keyboard::F2>() });
		i.insert({ "f3", Keyboard<sf::Keyboard::F3>() });
		i.insert({ "f4", Keyboard<sf::Keyboard::F4>() });
		i.insert({ "f5", Keyboard<sf::Keyboard::F5>() });
		i.insert({ "f6", Keyboard<sf::Keyboard::F6>() });
		i.insert({ "f7", Keyboard<sf::Keyboard::F7>() });
		i.insert({ "f8", Keyboard<sf::Keyboard::F8>() });
		i.insert({ "f9", Keyboard<sf::Keyboard::F9>() });
		i.insert({ "f10", Keyboard<sf::Keyboard::F10>() });
		i.insert({ "f11", Keyboard<sf::Keyboard::F11>() });
		i.insert({ "f12", Keyboard<sf::Keyboard::F12>() });
		//???
		i.insert({ "f13", Keyboard<sf::Keyboard::F13>() });
		i.insert({ "f14", Keyboard<sf::Keyboard::F14>() });
		i.insert({ "f15", Keyboard<sf::Keyboard::F15>() });
		//pause
		i.insert({ "pause", Keyboard<sf::Keyboard::Pause>() });
		//======mouse buttons=======
		i.insert({ "mouseleft", MouseButton<sf::Mouse::Button::Left>(window) });
		i.insert({ "mouseright", MouseButton<sf::Mouse::Button::Right>(window) });
		i.insert({ "mousemiddle", MouseButton<sf::Mouse::Button::Middle>(window) });
		i.insert({ "mousex1", MouseButton<sf::Mouse::Button::XButton1>(window) });
		i.insert({ "mousex2", MouseButton<sf::Mouse::Button::XButton2>(window) });
		//mouse axis
		i.insert({ "mouse", {data::UniqueId(), nullptr, [&window](data::UniqueId id) {
			Action a;
			a.id = id;
			std::tie(a.x_axis, a.y_axis, a.active) = MousePos(window);

			return a;
		} } });
		//mouseMoveRelative
		//=====joy buttons=====
		//TODO: impliment for joystick
		//must set the 'current joystick'
		//joy axis

		//=====touch input=====
		//only support single touch
		//with build in functions
		i.insert({ "touch", {data::UniqueId(), nullptr,
			[&window](data::UniqueId id) {
			Action a;
			a.id = id;
			a.active = sf::Touch::isDown(0);
			auto pos = sf::Touch::getPosition(0, window);
			auto size = window.getSize();
			std::tie(a.x_axis, a.y_axis) = vector_clamp(pos.x, pos.y,
				0, 0,
				static_cast<types::int32>(size.x),
				static_cast<types::int32>(size.y));
			return a;
		}} });
	}

	void InputSystem::create(data::UniqueId action, bool rebindable)
	{
		_bindable.insert({ action, rebindable });
	}

	void InputSystem::create(data::UniqueId action, bool rebindable, types::string defaultBinding)
	{
		auto default_interpretor = _interpretors.find(defaultBinding);
		if (default_interpretor == _interpretors.end())
		{
			default_interpretor = _specialInterpretors.find(defaultBinding);
			//TODO: throw logic error here
			assert(default_interpretor != _specialInterpretors.end());
		}

		_inputMap.insert({ default_interpretor->second, action });
		_bindable.insert({ action, rebindable });
	}

	void InputSystem::addInterpretor(types::string name, InputInterpretor::event_function e, InputInterpretor::function f)
	{
		InputInterpretor i;
		i.eventCheck = e;
		i.statusCheck = f;
		i.id = data::UniqueId();

		auto out = _specialInterpretors.insert({ name, i });

		//already exists
		if (!out.second)
			throw std::logic_error("Tried to insert InputInterpretor with a name that has already been used: " + name);
	}

	bool InputSystem::bind(data::UniqueId action, types::string interpretor)
	{
		if (_bindable.find(action) == _bindable.end())
			return false;

		auto inter = _interpretors.find(interpretor);
		if (inter == _interpretors.end())
			return false;

		_inputMap.insert({ inter->second, action });
		return true;
	}

	void InputSystem::unbind(data::UniqueId action, types::string input)
	{
		auto inter = _interpretors.find(input);
		if (inter == _interpretors.end())
			return;

		auto bindings = _inputMap.equal_range(inter->second);
		_inputMap.erase(bindings.first, bindings.second);
	}

	void InputSystem::unbind(data::UniqueId action)
	{
		std::vector<input_map::iterator> erasure_list;
		for (auto it = _inputMap.begin(); it != _inputMap.end(); ++it)
		{
			if (it->second == action)
				erasure_list.push_back(it);
		}

		for (auto &it : erasure_list)
			_inputMap.erase(it);
	}

	void InputSystem::generateState(const std::vector<Event> &events)
	{
		InputSystem::action_set actionset;

		for (auto &i : _inputMap)
		{
			if (i.first.eventCheck)
			{
				bool handled = false;
				for (auto &e : events)
				{
					auto action = i.first.eventCheck(std::get<bool>(e), std::get<sf::Event>(e), i.second);
					if (std::get<bool>(action))
					{
						actionset.insert(std::get<Action>(action));
						handled = true;
					}
				}

				if (!handled)
				{
					auto prev = _previousState.find(i.second);
					if (prev == _previousState.end())
					{
						Action a;
						a.id = i.second;
						a.active = false;
						actionset.insert(a);
					}
					else
						actionset.insert(*prev);
				}
			}
			else if(i.first.statusCheck)
			{
				auto action = i.first.statusCheck(i.second);
				actionset.insert(action);
			}
		}

		actionset.swap(_previousState);
	}

	typename InputSystem::action_set InputSystem::getInputState() const
	{
		return _previousState;
	}
}
