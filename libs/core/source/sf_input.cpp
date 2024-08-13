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

using namespace std::string_view_literals;

namespace hades
{
	using id_type = input_interpreter::interpreter_id;
	
	using event_match_function = typename input_event_system::event_interpreter::event_match_function;
	using event_function = typename input_event_system::event_interpreter::event_function;
	
	using interpreter_funcs = std::tuple<event_match_function, event_function>;

	template<sf::Keyboard::Key k>
	static interpreter_funcs keyboard()
	{
		auto is_match = [](const sf::Event &e)->bool {
			if (e.type == sf::Event::KeyPressed ||
				e.type == sf::Event::KeyReleased)
				return e.key.code == k;

			return false;
		};

		auto event_check = [](const sf::Event &e, action)->action {
			if (e.type == sf::Event::KeyPressed)
				return make_action(true); //TODO: encode modifier key state in the x,y params

			return make_action(false); // KeyReleased
		};

		return { is_match, event_check };
	}

	static bool in_window(sf::Vector2i pos, const sf::Window &window) noexcept
	{
		auto size = window.getSize();
		return sf::IntRect{ {}, { integer_cast<int>(size.x), integer_cast<int>(size.y) } }.contains(pos);
	}

	static interpreter_funcs mouse_pos(const sf::Window &window)
	{
		auto is_match = [](const sf::Event &e)->bool {
			return e.type == sf::Event::MouseMoved;
		};

		auto event_check = [&window](const sf::Event &e, action) {
			action a;
			a.active = in_window({ e.mouseMove.x, e.mouseMove.y }, window);

			const auto mouse_move = vector::clamp(vector2<int>{ e.mouseMove.x, e.mouseMove.y },
				{ 0, 0 },
				{ static_cast<types::int32>(window.getSize().x),
				static_cast<types::int32>(window.getSize().y) });
			a.x_axis = mouse_move.x;
			a.y_axis = mouse_move.y;

			return a;
		};

		return { is_match, event_check };
	}

	template<sf::Mouse::Button b>
	static interpreter_funcs mouse_button(const sf::Window &window)
	{
		auto is_match = [](const sf::Event &e) {
			if (e.type == sf::Event::MouseButtonPressed ||
				e.type == sf::Event::MouseButtonReleased)
				return e.mouseButton.button == b;

			return false;
		};

		auto event_check = [&window](const sf::Event &e, action) {
			action a;

			if (e.type == sf::Event::MouseButtonPressed)
			{
				a.active = in_window({ e.mouseButton.x, e.mouseButton.y }, window);
				const auto mouse_pos = vector::clamp(vector2<int>{e.mouseButton.x, e.mouseButton.y},
					{ 0, 0 },
					{ static_cast<types::int32>(window.getSize().x),
					static_cast<types::int32>(window.getSize().y) });

				a.x_axis = mouse_pos.x;
				a.y_axis = mouse_pos.y;
			}
			else if (e.type == sf::Event::MouseButtonReleased)
			{
				const auto pos = vector::clamp(vector2<int>{e.mouseButton.x, e.mouseButton.y},
					{ 0, 0 },
					{ static_cast<types::int32>(window.getSize().x),
					static_cast<types::int32>(window.getSize().y) });
				a.x_axis = pos.x;
				a.y_axis = pos.y;
			}

			return a;
		};

		return { is_match, event_check };
	}

	static interpreter_funcs mouse_wheel(const sf::Window &window)
	{
		auto is_match = [](const sf::Event &e) {
			if (e.type == sf::Event::MouseWheelScrolled)
				return e.mouseWheelScroll.wheel == sf::Mouse::Wheel::Vertical;

			return false;
		};

		auto event_check = [&window](const sf::Event &e, action) {
			action a;
			assert(e.type == sf::Event::MouseWheelScrolled);
			a.active = in_window({ e.mouseWheelScroll.x, e.mouseWheelScroll.y }, window);
			a.y_axis = static_cast<int32>(e.mouseWheelScroll.delta * 100);

			return a;
		};

		return { is_match, event_check };
	}

	void register_sfml_input(const sf::Window &win, input_event_system &sys)
	{
		//TODO: use key symbols instead of names where appropriate
		// eg "-" instead of "dash"
		const auto insert = [&sys](std::pair<std::string_view, interpreter_funcs> f) {
			sys.add_interpreter(f.first, std::move(std::get<0>(f.second)), std::move(std::get<1>(f.second)));
		};

		//=====keyboard keys=====
		//alpha
		insert({ "a"sv, keyboard<sf::Keyboard::A>() });
		insert({ "b"sv, keyboard<sf::Keyboard::B>() });
		insert({ "c"sv, keyboard<sf::Keyboard::C>() });
		insert({ "d"sv, keyboard<sf::Keyboard::D>() });
		insert({ "e"sv, keyboard<sf::Keyboard::E>() });
		insert({ "f"sv, keyboard<sf::Keyboard::F>() });
		insert({ "g"sv, keyboard<sf::Keyboard::G>() });
		insert({ "h"sv, keyboard<sf::Keyboard::H>() });
		insert({ "i"sv, keyboard<sf::Keyboard::I>() });
		insert({ "j"sv, keyboard<sf::Keyboard::J>() });
		insert({ "k"sv, keyboard<sf::Keyboard::K>() });
		insert({ "l"sv, keyboard<sf::Keyboard::L>() });
		insert({ "m"sv, keyboard<sf::Keyboard::M>() });
		insert({ "n"sv, keyboard<sf::Keyboard::N>() });
		insert({ "o"sv, keyboard<sf::Keyboard::O>() });
		insert({ "p"sv, keyboard<sf::Keyboard::P>() });
		insert({ "q"sv, keyboard<sf::Keyboard::Q>() });
		insert({ "r"sv, keyboard<sf::Keyboard::R>() });
		insert({ "s"sv, keyboard<sf::Keyboard::S>() });
		insert({ "t"sv, keyboard<sf::Keyboard::T>() });
		insert({ "u"sv, keyboard<sf::Keyboard::U>() });
		insert({ "v"sv, keyboard<sf::Keyboard::V>() });
		insert({ "w"sv, keyboard<sf::Keyboard::W>() });
		insert({ "x"sv, keyboard<sf::Keyboard::X>() });
		insert({ "y"sv, keyboard<sf::Keyboard::Y>() });
		insert({ "z"sv, keyboard<sf::Keyboard::Z>() });
		//numeric
		insert({ "0"sv, keyboard<sf::Keyboard::Num0>() });
		insert({ "1"sv, keyboard<sf::Keyboard::Num1>() });
		insert({ "2"sv, keyboard<sf::Keyboard::Num2>() });
		insert({ "3"sv, keyboard<sf::Keyboard::Num3>() });
		insert({ "4"sv, keyboard<sf::Keyboard::Num4>() });
		insert({ "5"sv, keyboard<sf::Keyboard::Num5>() });
		insert({ "6"sv, keyboard<sf::Keyboard::Num6>() });
		insert({ "7"sv, keyboard<sf::Keyboard::Num7>() });
		insert({ "8"sv, keyboard<sf::Keyboard::Num8>() });
		insert({ "9"sv, keyboard<sf::Keyboard::Num9>() });
		//control
		insert({ "escape"sv, keyboard<sf::Keyboard::Escape>() });
		insert({ "lcontrol"sv, keyboard<sf::Keyboard::LControl>() });
		insert({ "lshift"sv, keyboard<sf::Keyboard::LShift>() });
		insert({ "lalt"sv, keyboard<sf::Keyboard::LAlt>() });
		//i.insert({ "lsystem"sv, keyboard<sf::Keyboard::LSystem>() }); //don't rebind system key
		insert({ "rcontrol"sv, keyboard<sf::Keyboard::RControl>() });
		insert({ "rshift"sv, keyboard<sf::Keyboard::RShift>() });
		insert({ "ralt"sv, keyboard<sf::Keyboard::RAlt>() });
		//i.insert({ "rsystem"sv, keyboard<sf::Keyboard::RSystem>() });
		insert({ "menu"sv, keyboard<sf::Keyboard::Menu>() });
		//special
		insert({ "lbracket"sv, keyboard<sf::Keyboard::LBracket>() });
		insert({ "rbracket"sv, keyboard<sf::Keyboard::RBracket>() });
		insert({ "semicolon"sv, keyboard<sf::Keyboard::Semicolon>() });
		insert({ "comma"sv, keyboard<sf::Keyboard::Comma>() });
		insert({ "period"sv, keyboard<sf::Keyboard::Period>() });
		insert({ "quote"sv, keyboard<sf::Keyboard::Apostrophe>()});
		insert({ "slash"sv, keyboard<sf::Keyboard::Slash>() });
		insert({ "backslash"sv, keyboard<sf::Keyboard::Backslash>() });
		//i.insert({ "tilde"sv, keyboard<sf::Keyboard::Tilde>() }); // Reserved for engine usage
		insert({ "equal"sv, keyboard<sf::Keyboard::Equal>() });
		insert({ "dash"sv, keyboard<sf::Keyboard::Hyphen>() });
		insert({ "space"sv, keyboard<sf::Keyboard::Space>() });
		insert({ "return"sv, keyboard<sf::Keyboard::Enter>() });
		insert({ "backspace"sv, keyboard<sf::Keyboard::Backspace>() });
		insert({ "tab"sv, keyboard<sf::Keyboard::Tab>() });
		insert({ "pageup"sv, keyboard<sf::Keyboard::PageUp>() });
		insert({ "pagedown"sv, keyboard<sf::Keyboard::PageDown>() });
		insert({ "end"sv, keyboard<sf::Keyboard::End>() });
		insert({ "home"sv, keyboard<sf::Keyboard::Home>() });
		insert({ "insert"sv, keyboard<sf::Keyboard::Insert>() });
		insert({ "delete"sv, keyboard<sf::Keyboard::Delete>() });
		insert({ "add"sv, keyboard<sf::Keyboard::Add>() });
		//numpad
		insert({ "subtract"sv, keyboard<sf::Keyboard::Subtract>() });
		insert({ "multiply"sv, keyboard<sf::Keyboard::Multiply>() });
		insert({ "divide"sv, keyboard<sf::Keyboard::Divide>() });
		insert({ "left"sv, keyboard<sf::Keyboard::Left>() });
		insert({ "right"sv, keyboard<sf::Keyboard::Right>() });
		insert({ "up"sv, keyboard<sf::Keyboard::Up>() });
		insert({ "down"sv, keyboard<sf::Keyboard::Down>() });
		insert({ "num0"sv, keyboard<sf::Keyboard::Numpad0>() });
		insert({ "num1"sv, keyboard<sf::Keyboard::Numpad1>() });
		insert({ "num2"sv, keyboard<sf::Keyboard::Numpad2>() });
		insert({ "num3"sv, keyboard<sf::Keyboard::Numpad3>() });
		insert({ "num4"sv, keyboard<sf::Keyboard::Numpad4>() });
		insert({ "num5"sv, keyboard<sf::Keyboard::Numpad5>() });
		insert({ "num6"sv, keyboard<sf::Keyboard::Numpad6>() });
		insert({ "num7"sv, keyboard<sf::Keyboard::Numpad7>() });
		insert({ "num8"sv, keyboard<sf::Keyboard::Numpad8>() });
		insert({ "num9"sv, keyboard<sf::Keyboard::Numpad9>() });
		insert({ "f1"sv, keyboard<sf::Keyboard::F1>() });
		insert({ "f2"sv, keyboard<sf::Keyboard::F2>() });
		insert({ "f3"sv, keyboard<sf::Keyboard::F3>() });
		insert({ "f4"sv, keyboard<sf::Keyboard::F4>() });
		insert({ "f5"sv, keyboard<sf::Keyboard::F5>() });
		insert({ "f6"sv, keyboard<sf::Keyboard::F6>() });
		insert({ "f7"sv, keyboard<sf::Keyboard::F7>() });
		insert({ "f8"sv, keyboard<sf::Keyboard::F8>() });
		insert({ "f9"sv, keyboard<sf::Keyboard::F9>() });
		insert({ "f10"sv, keyboard<sf::Keyboard::F10>() });
		insert({ "f11"sv, keyboard<sf::Keyboard::F11>() });
		insert({ "f12"sv, keyboard<sf::Keyboard::F12>() });
		// Extended function keys (not available on most keyboards)
		//insert({ "f13"sv, keyboard<sf::Keyboard::F13>() });
		//insert({ "f14"sv, keyboard<sf::Keyboard::F14>() });
		//insert({ "f15"sv, keyboard<sf::Keyboard::F15>() });
		//pause
		insert({ "pause"sv, keyboard<sf::Keyboard::Pause>() });
		//======mouse buttons=======
		insert({ "mouseleft"sv, mouse_button<sf::Mouse::Button::Left>(win) });
		insert({ "mouseright"sv, mouse_button<sf::Mouse::Button::Right>(win) });
		// wheelbutton
		insert({ "mousemiddle"sv, mouse_button<sf::Mouse::Button::Middle>(win) });
		// mouse extra buttons
		insert({ "mouse_x1"sv, mouse_button<sf::Mouse::Button::Extra1>(win) });
		insert({ "mouse_x2"sv, mouse_button<sf::Mouse::Button::Extra2>(win) });
		//mouse axis
		insert({ "mouse"sv, mouse_pos(win) });
		insert({ "mousewheel"sv, mouse_wheel(win) });
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
	}
}
