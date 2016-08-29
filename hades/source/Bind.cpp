#include "Hades/Bind.hpp"

#include <algorithm>
#include <cassert>
#include <cctype> //for ::toupper
#include <functional>
#include <iterator>
#include <locale>
#include <string>
#include <sstream>

#include "Thor/Input/Action.hpp"
#include "Thor/Input/InputNames.hpp"
#include "Thor/Graphics/ToString.hpp"

namespace hades
{
	namespace
	{
		enum {SOURCE, ACTION, MODIFIER, EVENT};
		enum {JOYID = EVENT, JOYBUTTON};
		enum {JOYAXISID = MODIFIER, JOYAXIS};

		std::string key = "key", mouseButton = "mousebutton", mouseMove = "mousemove",
			joyButton = "joybutton", joyMove = "joymove", textEntered = "text";

		struct ExposedAction
		{
			thor::detail::ActionNode::CopiedPtr mOperation;
		};
	}

	void Bind::attach(sf::Window &window)
	{
		//_actions = std::unique_ptr< thor::ActionMap<int> >( new thor::ActionMap<int>(window) );
	}

	void Bind::attach(std::shared_ptr<Console> &console)
	{
		_console = console;
	}

	void Bind::dropEvents()
	{
		_actions.clearEvents();
	}

	void Bind::update(sf::Event &e)
	{
		_actions.pushEvent(e);
	}

	void Bind::sendEvents(CallbackSystem &callback, sf::Window &window)
	{
		_actions.invokeCallbacks(callback, &window);
	}

	void Bind::registerInputName(int id, std::string name)
	{
		_types[name] = id;
	}

	bool Bind::bindControl(std::string params)
	{
		std::vector<std::string> strings;

		std::stringstream ss(params);
		std::copy(std::istream_iterator<std::string>(ss), std::istream_iterator<std::string>(),
			std::back_inserter<std::vector<std::string> >(strings)); //split strings using space as delimiter

		if(strings.size() < 2)
			return false; //a command with less than 3 parameters isn't enought to define a command anyway

		auto type = _types.find(strings[ACTION]);
		if(type == _types.end())
			return false; //undefined action :(

		if(strings[SOURCE] == key)
		{
			if(strings.size() == 3)
				return bindKeyString(type->second, strings[MODIFIER], strings[EVENT]);
		}
		if(strings[SOURCE] == mouseButton)
		{
			if(strings.size() == 3)
				return bindMouseString(type->second, strings[MODIFIER], strings[EVENT]);
		}
		if(strings[SOURCE] == mouseMove)
		{
			return bindMouseMove(type->second);
		}
		if(strings[SOURCE] == joyButton)
		{
			if(strings.size() == 5)
				return bindJoyString(type->second, strings[MODIFIER], strings[JOYID], strings[JOYBUTTON]);
		}
		if(strings[SOURCE] == joyMove)
		{
			if(strings.size() == 4)
				return bindJoyMoveString(type->second, strings[JOYAXISID], strings[JOYAXIS]);
		}

		return false;
	}

	bool Bind::bindKeyString(int id, std::string modifier, std::string keyname)
	{
		//thor ::toKeyboardKey requires the input to be in uppercase, so we'll convert it
		std::transform(keyname.begin(), keyname.end(), keyname.begin(), (int (*)(int))std::toupper); //c syle cast to help gcc firgure out which ::toupper we want
		sf::Keyboard::Key key;
		try
		{
			key = thor::toKeyboardKey(keyname);
		}
		catch(thor::StringConversionException &e)
		{ 
			if (_console)
			{
				_console->echo("Bind key failed to convert key string: \"" + keyname + "\" to keycode.", Console::Console_String_Verbosity::ERROR);
				_console->echo("Thor exception contents: " + std::string(e.what()), Console::Console_String_Verbosity::WARNING);
			}

			return false;
		}

		int mod = PRESS;
		if(modifier == "hold")
			mod = HOLD;
		else if(modifier == "press")
			mod = PRESS;
		else if(modifier == "release")
			mod = RELEASE;

		return bindKey(id, mod, key);
	}

	bool Bind::bindKey(int id, int modifier, sf::Keyboard::Key key)
	{
		thor::Action action(key, convertToActionType(modifier));
		return bindAction(id, action);
	}

	bool Bind::bindMouseString(int id, std::string modifier, std::string keyname)
	{
		//thor ::toKeyboardKey requires the input to be in uppercase, so we'll convert it
		std::transform(keyname.begin(), keyname.end(), keyname.begin(), (int (*)(int))std::toupper);
		sf::Mouse::Button key;
		try{
			key = thor::toMouseButton(keyname);
		}
		catch (thor::StringConversionException &e)
		{
			if (_console)
			{
				_console->echo("Bind key failed to convert mouse string: \"" + keyname + "\" to keycode.", Console::Console_String_Verbosity::ERROR);
				_console->echo("Thor exception contents: " + std::string(e.what()), Console::Console_String_Verbosity::WARNING);
			}

			return false;
		}

		int mod = PRESS;
		if(modifier == "hold")
			mod = HOLD;
		else if(modifier == "press")
			mod = PRESS;
		else if(modifier == "release")
			mod = RELEASE;

		return bindMouse(id, mod, key);
	}

	bool Bind::bindMouse(int id, int modifier, sf::Mouse::Button button)
	{
		thor::Action action(button, convertToActionType(modifier));
		return bindAction(id, action);
	}

	bool Bind::bindJoyString(int id, std::string modifier, std::string joyStickId, std::string keyname)
	{
		//convert to jostick event
		std::stringstream strStk(joyStickId);
		std::stringstream strBtn(keyname);
		int joyId, joyButton;
		
		strStk >> joyId;
		strBtn >> joyButton;

		if(!strStk || !strBtn)
			return false; //invalid strings passed to bindJoy

		auto joyEvent = thor::joystick(joyId).button(joyButton);

		int mod = PRESS;
		if(modifier == "hold")
			mod = HOLD;
		else if(modifier == "press")
			mod = PRESS;
		else if(modifier == "release")
			mod = RELEASE;

		if (id == -1)
			id = sf::Joystick::Count + 1;

		return bindJoy(id, mod, joyEvent);
	}	

	bool Bind::bindJoy(int id, int modifier, thor::JoystickButton joyButton)
	{
		//assert(_actions);

		bool all = false;

		if (joyButton.joystickId == sf::Joystick::Count + 1)
			all = true;
		else if (joyButton.joystickId > sf::Joystick::Count)
			return false;

		if (joyButton.button > sf::Joystick::ButtonCount)
			return false;

		if (all)
		{
			for (int i = 0; i <= sf::Joystick::Count; ++i)
			{
				joyButton.joystickId = i;
				thor::Action action(joyButton, convertToActionType(modifier));
				bindAction(id, action);
			}
		}
		else
		{
			thor::Action action(joyButton, convertToActionType(modifier));
			return bindAction(id, action);
		}

		return true;
	}

	bool Bind::bindMouseMove(int id)
	{
		thor::Action action(sf::Event::MouseMoved);
		return bindAction(id, action);
	}

	bool Bind::bindTextEntered(int id)
	{
		thor::Action action(sf::Event::TextEntered);
		return bindAction(id, action);
	}

	bool Bind::bindAnyJoyMove(int id)
	{
		thor::Action action(sf::Event::JoystickMoved);
		return bindAction(id, action);
	}

	bool Bind::bindJoyMoveString(int id, std::string joyStickId, std::string joyStickAxis)
	{
		//convert to jostick event
		std::stringstream strStk(joyStickId);
		int joyId;
		
		strStk >> joyId;
		if(!strStk)
			return false; //invalid strings passed to bindJoy

		std::transform(joyStickAxis.begin(), joyStickAxis.end(), joyStickAxis.begin(), (int (*)(int))std::toupper);

		//thor::JoystickAxis axis;
		try{
			//axis = thor::toJoystickAxis(joyStickAxis);
		}
		catch (thor::StringConversionException &e)
		{
			if (_console)
			{
				_console->echo("Bind key failed to convert joystick string: \"" + joyStickAxis + "\" to keycode.", Console::Console_String_Verbosity::ERROR);
				_console->echo("Thor exception contents: " + std::string(e.what()), Console::Console_String_Verbosity::WARNING);
			}

			return false;
		}

		//return bindJoyMove(id, joyId, axis);
		return false;
	}
		
	bool Bind::bindJoyMove(int id, thor::JoystickAxis axis)
	{
		bool all = false;

		if (axis.joystickId == sf::Joystick::Count + 1)
			all = true;
		else if (axis.joystickId > sf::Joystick::Count)
			return false;

		if (all)
		{
			for (int i = 0; i <= sf::Joystick::Count; ++i)
			{
				axis.joystickId = i;
				thor::Action action(axis);
				bindAction(id, action);
			}
		}
		else
		{
			thor::Action action(axis);
			return bindAction(id, action);
		}

		return true;
	}

	void Bind::unbind(int id)
	{
		_actions.removeAction(id);
	}

	void Bind::unbindAll()
	{
		_actions.clearActions();
	}

	bool Bind::bindAction(int id, thor::Action &action)
	{
		auto currentAction = _actions[id];

		auto expose = reinterpret_cast<ExposedAction&>(currentAction);

		if(expose.mOperation)
			_actions[id] = currentAction || action;
		else
			_actions[id] = action;
			 
		//_callbacks.connect(id, std::bind(_bindFunction, std::placeholders::_1, id));

		return true;
	}

	thor::Action::ActionType Bind::convertToActionType(int type)
	{
		switch(type)
		{
		default:
		case HOLD:
			return thor::Action::Hold;
			break;
		case PRESS:
			return thor::Action::PressOnce;
			break;
		case RELEASE:
			return thor::Action::ReleaseOnce;
		}
	}
}
