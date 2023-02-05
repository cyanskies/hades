#include "hades/sf_input.hpp"

#include <map>
#include <tuple>

#include "SFML/Graphics/Rect.hpp"
#include "SFML/Window/Keyboard.hpp"
#include "SFML/Window/Mouse.hpp"
#include "SFML/Window/Touch.hpp"
#include "SFML/Window/Window.hpp"

#include "hades/uniqueid.hpp"
#include "hades/vector_math.hpp"

namespace hades
{
	using id_type = input_interpreter::interpreter_id;
	
	using event_match_function = typename input_event_system::event_interpreter::event_match_function;
	using event_function = typename input_event_system::event_interpreter::event_function;
	
	using interpreter_funcs = std::tuple<event_match_function, event_function, input_interpreter::function>;

	template<sf::Keyboard::Key k>
	interpreter_funcs keyboard()
	{
		auto realtime = [](action prev)->action {
			const auto pressed = sf::Keyboard::isKeyPressed(k);
			if (prev.active == pressed)
				return prev;
			return make_action(pressed);
		};

		auto is_match = [](const sf::Event &e)->bool {
			if (e.type == sf::Event::KeyPressed ||
				e.type == sf::Event::KeyReleased)
				return e.key.code == k;

			return false;
		};

		auto event_check = [](bool handled, const sf::Event &e, action)->action {
			if (handled)
				return make_action(false);
			
			if (e.type == sf::Event::KeyPressed)
				return make_action(true); //TODO: encode modifier key state in the x,y params

			return make_action(false); // KeyReleased
		};

		return { is_match, event_check, nullptr /*realtime*/};
	}

	bool in_window(sf::Vector2i pos, const sf::Window &window)
	{
		auto size = window.getSize();
		return sf::IntRect{ {}, { integer_cast<int>(size.x), integer_cast<int>(size.y) } }.contains(pos);
	}

	interpreter_funcs mouse_pos(const sf::Window &window)
	{
		auto is_match = [](const sf::Event &e)->bool {
			return e.type == sf::Event::MouseMoved;
		};

		auto event_check = [&window](bool handled, const sf::Event &e, action) {
			action a;

			if (!handled)
				a.active = in_window({ e.mouseMove.x, e.mouseMove.y }, window);
			else
				a.active = false;

			const auto mouse_move = vector::clamp(vector_t<int>{ e.mouseMove.x, e.mouseMove.y },
				{ 0, 0 },
				{ static_cast<types::int32>(window.getSize().x),
				static_cast<types::int32>(window.getSize().y) });
			a.x_axis = mouse_move.x;
			a.y_axis = mouse_move.y;

			return a;
		};

		auto realtime_check = [&window](action) {
			action a;

			auto mpos = sf::Mouse::getPosition(window);
			auto size = window.getSize();
			a.active = sf::IntRect{ {}, static_cast<sf::Vector2i>(size) }.contains(mpos);

			const auto mouse_pos = vector::clamp(vector_t<int>{ mpos.x, mpos.y },
				{ 0, 0 },
				{ static_cast<types::int32>(size.x),
				static_cast<types::int32>(size.y) });

			a.x_axis = mouse_pos.x;
			a.y_axis = mouse_pos.y;

			return a;
		};

		return { is_match, event_check, nullptr /*realtime_check*/};
	}

	template<sf::Mouse::Button b>
	interpreter_funcs mouse_button(const sf::Window &window)
	{
		auto is_match = [](const sf::Event &e) {
			if (e.type == sf::Event::MouseButtonPressed ||
				e.type == sf::Event::MouseButtonReleased)
				return e.mouseButton.button == b;

			return false;
		};

		auto event_check = [&window](bool handled, const sf::Event &e, action) {
			action a;

			if (!handled && e.type == sf::Event::MouseButtonPressed)
			{
				a.active = in_window({ e.mouseButton.x, e.mouseButton.y }, window);
				const auto mouse_pos = vector::clamp(vector_t<int>{e.mouseButton.x, e.mouseButton.y},
					{ 0, 0 },
					{ static_cast<types::int32>(window.getSize().x),
					static_cast<types::int32>(window.getSize().y) });

				a.x_axis = mouse_pos.x;
				a.y_axis = mouse_pos.y;
			}
			else if (e.type == sf::Event::MouseButtonReleased)
			{
				a.active = false;
				const auto pos = vector::clamp(vector_t<int>{e.mouseButton.x, e.mouseButton.y},
					{ 0, 0 },
					{ static_cast<types::int32>(window.getSize().x),
					static_cast<types::int32>(window.getSize().y) });
				a.x_axis = pos.x;
				a.y_axis = pos.y;
			}

			return a;
		};

		auto realtime_check = [&window](action) {
			action a;

			const auto m_pos = sf::Mouse::getPosition(window);
			a.active = sf::Mouse::isButtonPressed(b) && in_window(m_pos, window);; 
			const auto size = window.getSize();
			const auto pos = vector::clamp(vector_t<int>{m_pos.x, m_pos.y},
				{ 0, 0 },
				{ static_cast<types::int32>(size.x),
				static_cast<types::int32>(size.y) });
			a.x_axis = pos.x;
			a.y_axis = pos.y;

			return a;
		};

		return {is_match, event_check, nullptr};
	}

	interpreter_funcs mouse_wheel(const sf::Window &window)
	{
		auto is_match = [](const sf::Event &e) {
			if (e.type == sf::Event::MouseWheelScrolled)
				return e.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel;

			return false;
		};

		auto event_check = [&window](bool handled, const sf::Event &e, action) {
			action a;

			if (!handled && e.type == sf::Event::MouseWheelScrolled)
			{
				a.active = in_window({ e.mouseWheelScroll.x, e.mouseWheelScroll.y }, window);
				a.y_axis = static_cast<int32>(e.mouseWheelScroll.delta * 100);
			}

			return a;
		};

		return { is_match, event_check, nullptr };
	}

	void register_sfml_input(const sf::Window &win, input_event_system &sys)
	{
		//TODO: use key symbols instead of names where appropriate
		// eg "-" instead of "dash"

		std::map<types::string, interpreter_funcs> interpreter_map;
		//=====keyboard keys=====
		//alpha
		interpreter_map.insert({ "a", keyboard<sf::Keyboard::A>() });
		interpreter_map.insert({ "b", keyboard<sf::Keyboard::B>() });
		interpreter_map.insert({ "c", keyboard<sf::Keyboard::C>() });
		interpreter_map.insert({ "d", keyboard<sf::Keyboard::D>() });
		interpreter_map.insert({ "e", keyboard<sf::Keyboard::E>() });
		interpreter_map.insert({ "f", keyboard<sf::Keyboard::F>() });
		interpreter_map.insert({ "g", keyboard<sf::Keyboard::G>() });
		interpreter_map.insert({ "h", keyboard<sf::Keyboard::H>() });
		interpreter_map.insert({ "i", keyboard<sf::Keyboard::I>() });
		interpreter_map.insert({ "j", keyboard<sf::Keyboard::J>() });
		interpreter_map.insert({ "k", keyboard<sf::Keyboard::K>() });
		interpreter_map.insert({ "l", keyboard<sf::Keyboard::L>() });
		interpreter_map.insert({ "m", keyboard<sf::Keyboard::M>() });
		interpreter_map.insert({ "n", keyboard<sf::Keyboard::N>() });
		interpreter_map.insert({ "o", keyboard<sf::Keyboard::O>() });
		interpreter_map.insert({ "p", keyboard<sf::Keyboard::P>() });
		interpreter_map.insert({ "q", keyboard<sf::Keyboard::Q>() });
		interpreter_map.insert({ "r", keyboard<sf::Keyboard::R>() });
		interpreter_map.insert({ "s", keyboard<sf::Keyboard::S>() });
		interpreter_map.insert({ "t", keyboard<sf::Keyboard::T>() });
		interpreter_map.insert({ "u", keyboard<sf::Keyboard::U>() });
		interpreter_map.insert({ "v", keyboard<sf::Keyboard::V>() });
		interpreter_map.insert({ "w", keyboard<sf::Keyboard::W>() });
		interpreter_map.insert({ "x", keyboard<sf::Keyboard::X>() });
		interpreter_map.insert({ "y", keyboard<sf::Keyboard::Y>() });
		interpreter_map.insert({ "z", keyboard<sf::Keyboard::Z>() });
		//numeric
		interpreter_map.insert({ "0", keyboard<sf::Keyboard::Num0>() });
		interpreter_map.insert({ "1", keyboard<sf::Keyboard::Num1>() });
		interpreter_map.insert({ "2", keyboard<sf::Keyboard::Num2>() });
		interpreter_map.insert({ "3", keyboard<sf::Keyboard::Num3>() });
		interpreter_map.insert({ "4", keyboard<sf::Keyboard::Num4>() });
		interpreter_map.insert({ "5", keyboard<sf::Keyboard::Num5>() });
		interpreter_map.insert({ "6", keyboard<sf::Keyboard::Num6>() });
		interpreter_map.insert({ "7", keyboard<sf::Keyboard::Num7>() });
		interpreter_map.insert({ "8", keyboard<sf::Keyboard::Num8>() });
		interpreter_map.insert({ "9", keyboard<sf::Keyboard::Num9>() });
		//control
		interpreter_map.insert({ "escape", keyboard<sf::Keyboard::Escape>() });
		interpreter_map.insert({ "lcontrol", keyboard<sf::Keyboard::LControl>() });
		interpreter_map.insert({ "lshift", keyboard<sf::Keyboard::RControl>() });
		interpreter_map.insert({ "lalt", keyboard<sf::Keyboard::LAlt>() });
		//i.insert({ "lsystem", keyboard<sf::Keyboard::LSystem>() }); //don't rebind system key
		interpreter_map.insert({ "rcontrol", keyboard<sf::Keyboard::RControl>() });
		interpreter_map.insert({ "rshift", keyboard<sf::Keyboard::RShift>() });
		interpreter_map.insert({ "ralt", keyboard<sf::Keyboard::RAlt>() });
		//i.insert({ "rsystem", keyboard<sf::Keyboard::RSystem>() });
		interpreter_map.insert({ "menu", keyboard<sf::Keyboard::Menu>() });
		//special
		interpreter_map.insert({ "lbracket", keyboard<sf::Keyboard::LBracket>() });
		interpreter_map.insert({ "rbracket", keyboard<sf::Keyboard::RBracket>() });
		interpreter_map.insert({ "semicolon", keyboard<sf::Keyboard::Semicolon>() });
		interpreter_map.insert({ "comma", keyboard<sf::Keyboard::Comma>() });
		interpreter_map.insert({ "period", keyboard<sf::Keyboard::Period>() });
		interpreter_map.insert({ "quote", keyboard<sf::Keyboard::Quote>() });
		interpreter_map.insert({ "slash", keyboard<sf::Keyboard::Slash>() });
		interpreter_map.insert({ "backslash", keyboard<sf::Keyboard::Backslash>() });
		//i.insert({ "tilde", keyboard<sf::Keyboard::Tilde>() }); // Reserved for engine usage
		interpreter_map.insert({ "equal", keyboard<sf::Keyboard::Equal>() });
		interpreter_map.insert({ "dash", keyboard<sf::Keyboard::Hyphen>() });
		interpreter_map.insert({ "space", keyboard<sf::Keyboard::Space>() });
		interpreter_map.insert({ "return", keyboard<sf::Keyboard::Enter>() });
		interpreter_map.insert({ "backspace", keyboard<sf::Keyboard::Backspace>() });
		interpreter_map.insert({ "tab", keyboard<sf::Keyboard::Tab>() });
		interpreter_map.insert({ "pageup", keyboard<sf::Keyboard::PageUp>() });
		interpreter_map.insert({ "pagedown", keyboard<sf::Keyboard::PageDown>() });
		interpreter_map.insert({ "end", keyboard<sf::Keyboard::End>() });
		interpreter_map.insert({ "home", keyboard<sf::Keyboard::Home>() });
		interpreter_map.insert({ "insert", keyboard<sf::Keyboard::Insert>() });
		interpreter_map.insert({ "delete", keyboard<sf::Keyboard::Delete>() });
		interpreter_map.insert({ "add", keyboard<sf::Keyboard::Add>() });
		//numpad
		interpreter_map.insert({ "subtract", keyboard<sf::Keyboard::Subtract>() });
		interpreter_map.insert({ "multiply", keyboard<sf::Keyboard::Multiply>() });
		interpreter_map.insert({ "divide", keyboard<sf::Keyboard::Divide>() });
		interpreter_map.insert({ "left", keyboard<sf::Keyboard::Left>() });
		interpreter_map.insert({ "right", keyboard<sf::Keyboard::Right>() });
		interpreter_map.insert({ "up", keyboard<sf::Keyboard::Up>() });
		interpreter_map.insert({ "down", keyboard<sf::Keyboard::Down>() });
		interpreter_map.insert({ "num0", keyboard<sf::Keyboard::Numpad0>() });
		interpreter_map.insert({ "num1", keyboard<sf::Keyboard::Numpad1>() });
		interpreter_map.insert({ "num2", keyboard<sf::Keyboard::Numpad2>() });
		interpreter_map.insert({ "num3", keyboard<sf::Keyboard::Numpad3>() });
		interpreter_map.insert({ "num4", keyboard<sf::Keyboard::Numpad4>() });
		interpreter_map.insert({ "num5", keyboard<sf::Keyboard::Numpad5>() });
		interpreter_map.insert({ "num6", keyboard<sf::Keyboard::Numpad6>() });
		interpreter_map.insert({ "num7", keyboard<sf::Keyboard::Numpad7>() });
		interpreter_map.insert({ "num8", keyboard<sf::Keyboard::Numpad8>() });
		interpreter_map.insert({ "num9", keyboard<sf::Keyboard::Numpad9>() });
		interpreter_map.insert({ "f1", keyboard<sf::Keyboard::F1>() });
		interpreter_map.insert({ "f2", keyboard<sf::Keyboard::F2>() });
		interpreter_map.insert({ "f3", keyboard<sf::Keyboard::F3>() });
		interpreter_map.insert({ "f4", keyboard<sf::Keyboard::F4>() });
		interpreter_map.insert({ "f5", keyboard<sf::Keyboard::F5>() });
		interpreter_map.insert({ "f6", keyboard<sf::Keyboard::F6>() });
		interpreter_map.insert({ "f7", keyboard<sf::Keyboard::F7>() });
		interpreter_map.insert({ "f8", keyboard<sf::Keyboard::F8>() });
		interpreter_map.insert({ "f9", keyboard<sf::Keyboard::F9>() });
		interpreter_map.insert({ "f10", keyboard<sf::Keyboard::F10>() });
		interpreter_map.insert({ "f11", keyboard<sf::Keyboard::F11>() });
		interpreter_map.insert({ "f12", keyboard<sf::Keyboard::F12>() });
		// Extended function keys (not available on most keyboards)
		//interpreter_map.insert({ "f13", keyboard<sf::Keyboard::F13>() });
		//interpreter_map.insert({ "f14", keyboard<sf::Keyboard::F14>() });
		//interpreter_map.insert({ "f15", keyboard<sf::Keyboard::F15>() });
		//pause
		interpreter_map.insert({ "pause", keyboard<sf::Keyboard::Pause>() });
		//======mouse buttons=======
		interpreter_map.insert({ "mouseleft", mouse_button<sf::Mouse::Button::Left>(win) });
		interpreter_map.insert({ "mouseright", mouse_button<sf::Mouse::Button::Right>(win) });
		// wheelbutton
		interpreter_map.insert({ "mousemiddle", mouse_button<sf::Mouse::Button::Middle>(win) });
		// mouse extra buttons
		interpreter_map.insert({ "mouse_x1", mouse_button<sf::Mouse::Button::XButton1>(win) });
		interpreter_map.insert({ "mouse_x2", mouse_button<sf::Mouse::Button::XButton2>(win) });
		//mouse axis
		interpreter_map.insert({ "mouse", mouse_pos(win) });
		interpreter_map.insert({ "mousewheel", mouse_wheel(win) });
		//mouseMoveRelative
		//=====joy buttons=====
		//TODO: impliment for joystick
		//must set the 'current joystick'
		//joy axis
		/*
		//=====touch input=====
		//only support single touch
		//with build in functions
		i.insert({ "touch",{ unique_id(), nullptr,
			[&window]() {
			Action a;
			a.active = sf::Touch::isDown(0);
			auto pos = sf::Touch::getPosition(0, window);
			auto size = window.getSize();
			std::tie(a.x_axis, a.y_axis) = vector::clamp(pos.x, pos.y,
				0, 0,
				static_cast<types::int32>(size.x),
				static_cast<types::int32>(size.y));
			return a;
		} } });
		*/

		for (const auto &inter : interpreter_map)
			sys.add_interpreter(inter.first, std::get<0>(inter.second), std::get<1>(inter.second), std::get<2>(inter.second));
	}
}
