#include "Hades/App.hpp"

#include <cassert>
#include <string>

#include "SFML/System/Clock.hpp"
#include "SFML/Window/Event.hpp"

#include "Hades/Console.hpp"
#include "Hades/ResourceManager.hpp"

namespace hades
{
	//true float values for common ratios * 100
	const int RATIO3_4 = 133, RATIO16_9 = 178, RATIO16_10 = 160;

	void registerVariables(std::shared_ptr<Console> &console)
	{
		//console variables
		console->set<std::string>("con_characterfont", "console/console.ttf");
		console->set("con_charactersize", 15);
		console->set("con_fade", 180);

		//app variables
		console->set("tickrate", 30);
	}

	void registerVidVariables(std::shared_ptr<Console> &console)
	{
		std::function<bool(std::string)> vid_default = [&console] (std::string)->bool {
			//window
			console->set("vid_fullscreen", false);
			//resolution
			console->set("vid_width", 800);
			console->set("vid_height", 600);
			console->set("vid_mode", 0);
			//depth
			console->set("vid_depth", 32);

			return true;
		};

		//set the default now
		vid_default("");

		//add a command so that this function can be called again
		console->registerFunction("vid_default", vid_default);
	}

	void registerServerVariables(std::shared_ptr<Console> &console)
	{
		console->set("s_tickrate", 30);
		console->set("s_maxframetime", 150); // 1.5 seconds is the maximum allowable frame time.
		console->set("s_connectionport", 0); // 0 = auto port
		console->set("s_portrange", 0); // unused
	}

	void App::init()
	{
		//load config files
		_bindings.attach(_window);

		//bind console commands
		_bindings.registerInputName(CONSOLE_TOGGLE, "console_toggle");
		_bindings.registerInputName(TEXTENTERED, "text_entered");

		_bindings.bindKey(CONSOLE_TOGGLE, Bind::PRESS, sf::Keyboard::Tilde);
		_bindings.bindTextEntered(TEXTENTERED);

		defaultBindings(_bindings);

		//create console
		_console = std::make_shared<Console>();

		//load defualt console settings
		registerVariables(_console);
		registerVidVariables(_console);
		registerServerVariables(_console);

		_resource = std::make_shared<ResourceManager>();
		_resource->attach(_console);

		_resource->appendSystemPath("indev/");
		_resource->appendSystemPath("common/");
		
		_consoleView.attach(_console);
		_consoleView.attach(_resource);

		_consoleView.init();

		//load config files and overwrite any updated settings
		_states.attach(_console);
		_states.attach(_resource);

		registerConsoleCommands();		
	}

	void App::postInit(CommandList commands)
	{	
		if(!_console->runCommand("vid_reinit"))
		{
			_console->echo("Error setting video, falling back to default", Console::ERROR);
			_console->runCommand("vid_default");
			_console->runCommand("vid_reinit");
		}

		//if hades main handles any of the commands then they will be removed from 'commands'
		hadesMain(_states, _console, commands);

		//proccess config files
		//load saved bindings and whatnot from config files

		//process command lines
		//pass the commands into the console to be fullfilled using the normal parser
	}

	bool App::run()
	{
		auto tickRate = _console->getValue<int>("tickrate"),
			maxframetime  = _console->getValue<int>("s_maxframetime");

		assert(tickRate && "failed to get tick rate value from console");

		sf::Clock time;
		time.restart();
		bool running = true;

		sf::Time accumulator = sf::Time::Zero;

		while(running && _window.isOpen())
		{
			State *activeState = _states.getActiveState();
			if(!activeState)
				break;

			sf::Time thisFrame = sf::Time::Zero;
			sf::Time currentTime = time.getElapsedTime();
			//perform additional logic updates if we're behind on logic
			while( accumulator >= sf::milliseconds(tickRate->load()))
			{
				handleEvents(activeState);
				_bindings.sendEvents(activeState->getCallbackHandler(), _window);

				activeState->update(_window);
				accumulator -= sf::milliseconds(tickRate->load());
				thisFrame += sf::milliseconds(tickRate->load());
			}

			activeState->update(thisFrame.asSeconds());
			_window.clear();
			activeState->draw(_window);

			//render the console interface if it is active.
			if (_consoleView.active())
			{
				_consoleView.update();
				_consoleView.draw(_window);
			}

			_window.display();

			sf::Time maxFrameTime = sf::milliseconds(maxframetime->load());
			//cap the maximum time between frames, to stop the accumulator expanding too high.
			currentTime = time.getElapsedTime() - currentTime;
			if (currentTime > maxFrameTime)
				currentTime = maxFrameTime;

			accumulator += currentTime;
		}

		return EXIT_SUCCESS;
	}

	void App::cleanUp()
	{
		//close the client window
		if(_window.isOpen())
			_window.close();

		//kill off any lingering states.
		_states.drop();
	}

	void App::handleEvents(State *activeState)
	{
		//drop events from last frame
		_bindings.dropEvents();

		sf::Event e;
		while (_window.pollEvent(e))
		{
			//window events
			if (e.type == sf::Event::Closed) // if the app is closed, then disappear without a fuss
				_window.close();
			else if (e.type == sf::Event::KeyPressed && // otherwise check for console summon
				e.key.code == sf::Keyboard::Tilde)
				_consoleView.toggleActive();
			else if (e.type == sf::Event::Resized)	// handle resize before _consoleView, so that opening the console
			{										// doesn't block resizing the window
				_console->set<int>("vid_width", e.size.width);
				_console->set<int>("vid_height", e.size.height);
				_console->set("vid_mode", -1);
				_consoleView.init();
				activeState->handleEvent(e);		// let the gamestate see the changed window size
			}
			else if (_consoleView.active())	// if the console is active forward all input to it rather than the gamestate
			{
				if (e.type == sf::Event::KeyPressed &&
					e.key.code == sf::Keyboard::Return)
					_consoleView.sendCommand();
				else if (e.type == sf::Event::TextEntered)
					_consoleView.enterText(EventContext(&_window, &e, TEXTENTERED));
			}
			//input events
			else if(!activeState->handleEvent(e)) // otherwise let the gamestate handle it
				_bindings.update(e);				//if the state doesn't handle the event directly
													// then let the binding manager pass it as user input
		}
	}

	void App::registerConsoleCommands()
	{
		//general functions
		{
			std::function<bool(std::string)> exit = [this](std::string)->bool {
				_window.close();
				return true;
			};
			//exit and quit allow states, players or scripts to close the engine.
			_console->registerFunction("exit", exit);
			_console->registerFunction("quit", exit);

			_console->registerFunction("bind", std::bind(&Bind::bindControl, &_bindings, std::placeholders::_1));
		}

		//vid functions
		{
			std::function<bool(std::string)> vid_reinit = [this](std::string)->bool {
				auto width = _console->getValue<int>("vid_width"),
					height = _console->getValue<int>("vid_height"),
					depth = _console->getValue<int>("vid_depth");

				auto fullscreen = _console->getValue<bool>("vid_fullscreen");

				sf::VideoMode mode(width->load(), height->load(), depth->load());

				// if we're in fullscreen mode, then the videomode must be 'valid'
				if (fullscreen->load() && !mode.isValid())
				{
					_console->echo("Attempted to set invalid video mode.", Console::ERROR);
					return false;
				}

				float ratio = static_cast<float>(width->load()) / static_cast<float>(height->load());
				int intratio = static_cast<int>(ratio * 100);

				switch (intratio)
				{
				case RATIO3_4:
					_console->set("vid_mode", 0);
					break;
				case RATIO16_9:
					_console->set("vid_mode", 1);
					break;
				case RATIO16_10:
					_console->set("vid_mode", 2);
					break;
				default:
					_console->set("vid_mode", -1);
					_console->echo("Cannot ditermine video ratio.", Console::WARNING);
					break;
				}

				if (fullscreen->load())
					_window.create(mode, "game", sf::Style::Fullscreen);
				else
					_window.create(mode, "game", sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize);

				return true;
			};

			_console->registerFunction("vid_reinit", vid_reinit);
		}

		//utility functions
		{
			std::function<bool(std::string)> util_dir = [this](std::string path)->bool {
				auto files = _resource->listFilesInDirectory(path);

				for (auto f : files)
					_console->echo(f);

				return true;
			};

			_console->registerFunction("dir", util_dir);
		}

	}
}//hades