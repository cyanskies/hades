#ifndef HADES_BIND_HPP
#define HADES_BIND_HPP

#include <map>
#include <string>

#include "SFML/Window/Event.hpp"
#include "SFML/Window/Window.hpp"

#include "Thor/Input/Action.hpp"
#include "Thor/Input/ActionMap.hpp"
#include "Thor/Input/Connection.hpp"
#include "Thor/Input/EventSystem.hpp"

#include "Hades/Console.hpp"

namespace hades
{
	enum APP_INPUT { CONSOLE_TOGGLE = std::numeric_limits<int>::min(), TEXTENTERED };

	typedef thor::ActionMap<int>::CallbackSystem CallbackSystem;
	typedef thor::ActionContext<int> EventContext;
	//handles dynamic binding of input events to specific function calls
	class Bind
	{
	public:
		enum {HOLD, PRESS, RELEASE};

		//this is especially important, it not only attaches the window,
		//but also creates the actionmap
		void attach(sf::Window &window);
		void attach(std::shared_ptr<Console> &console);

		void dropEvents();
		void update(sf::Event &e);
		void sendEvents(CallbackSystem &callback, sf::Window &window);

		//type is one of the values in the above enum
		//id is unique to each string name
		//string name is used to update or change the binding
		void registerInputName(int id, std::string name);

		//binds a control using the console command

		// example usage: For buttons.
		// bind <source> <action> <modifier> <button>
		// bind key use hold a
		// bind mouse use hold left

		// example usage: For mouse movement.
		// bind <source> <action>
		// bind mousemove move

		// example usage: For joystick buttons
		// bind joy <acction> <modifier> <joypad number>
		// bind joy use hold 1

		// hypothetical usage: For joystick movement.
		// bind joymove <action> <joypad number> <axis>
		// bind joymove move 1 U

		// special actions can be bound to joysticks or mice
		// but I haven't worked out a good console command for them yet...
		bool bindControl(std::string params);

		bool bindKeyString(int id, std::string modifier, std::string keyname);
		bool bindKey(int id, int modifier, sf::Keyboard::Key key);
		//these are currently disabled
		//TODO: implement support of mice motion, buttons, joysticks and controller buttons.
		bool bindMouseString(int id, std::string modifier, std::string keyname);
		bool bindMouse(int id, int modifier, sf::Mouse::Button button);
		
		bool bindJoyString(int id, std::string modifier, std::string joyStickId, std::string keyname);
		bool bindJoy(int id, int modifier, thor::JoystickButton joyButton);

		//binds a joystick button for all joysticks,
		//app must check the eventcontext to find the joystick that caused it
		//bool bindAnyJoyString(int id, int modifier, int button);
		bool bindMouseMove(int id);
		bool bindTextEntered(int id);

		bool bindAnyJoyMove(int id);

		//not supported yet
		bool bindJoyMoveString(int id, std::string joyStickId, std::string joyStickAxis);
		bool bindJoyMove(int id, thor::JoystickAxis axis);

		void unbind(int id);
		void unbindAll();	

	private:
		bool bindAction(int id, thor::Action &action);
		thor::Action::ActionType convertToActionType(int type);

		//TODO: unordered map
		std::map<std::string, int > _types;
		thor::ActionMap<int> _actions;

		std::shared_ptr<Console> _console;
	};
} //hades
#endif //HADES_BIND_HPP
