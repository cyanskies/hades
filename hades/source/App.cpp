#include "Hades/App.hpp"

#include <cassert>
#include <string>
#include <sstream>

#include "SFML/System/Clock.hpp"
#include "SFML/Window/Event.hpp"

#include "zlib/zlib.h" //for zlib version

#include "Hades/Console.hpp"
#include "Hades/Logging.hpp"
#include "Hades/parallel_jobs.hpp"
#include "Hades/Properties.hpp"

namespace hades
{
	//true float values for common ratios * 100
	const int RATIO3_4 = 133, RATIO16_9 = 178, RATIO16_10 = 160;

	//console variable names
	//c_* client variables names
	// for variables that control basic client behaviour: framerate, etc
	//cons_* console variable names
	// for console behaviour, font type, size and so on
	//r_* render variable names
	// none currently?
	//s_* server variables
	// server framerate and so on
	//vid_* video settings variables
	// resolution, colour depth, fullscreen, etc

	void registerVariables(std::shared_ptr<Console> &console)
	{
		//console variables
		console->set<std::string>("con_characterfont", "console/console.ttf");
		console->set("con_charactersize", 15);
		console->set("con_fade", 180);

		//app variables
		console->set("c_ticktime", 30);
		console->set("c_maxticktime", 150); // 1.5 seconds is the maximum allowable tick time.

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
		console->registerFunction("vid_default", vid_default, true);
	}

	void registerServerVariables(std::shared_ptr<Console> &console)
	{
		console->set("s_tickrate", 30);
		console->set("s_maxframetime", 150); // 1.5 seconds is the maximum allowable frame time.
		console->set("s_connectionport", 0); // 0 = auto port
		console->set("s_portrange", 0); // unused
	}

	App::App() : _input(_window)
	{}

	void App::init()
	{
		//create console
		_console = std::make_shared<Console>();
		//record the global console as logger
		console::log = &*_console;
		//record the console as the property provider
		console::property_provider = &*_console;
		//record the console as the engine command line
		console::system_object = &*_console;

		//record the global resource controller
		data_manager = &_dataMan;

		resourceTypes(_dataMan);

		//load defualt console settings
		registerVariables(_console);
		registerVidVariables(_console);
		registerServerVariables(_console);
		
		_consoleView.init();

		//load config files and overwrite any updated settings
		_states.setGuiTarget(_window);

		registerConsoleCommands();	

		//initialise the job system
		parallel_jobs::init();
	}

	//calls job on any command that contains command
	void loadCommand(CommandList &commands, std::string command, std::function<void(CommandList::value_type)> job)
	{
		auto bit = commands.begin(), end = commands.end();
		std::vector<CommandList::iterator> removeList;
		while (bit != end)
		{
			auto com = bit->substr(0, command.length());

			if (com == command)
			{
				removeList.push_back(bit);
				job(*bit);
			}
			++bit;
		}

		while (!removeList.empty())
		{
			commands.erase(removeList.back());
			removeList.pop_back();
		}
	}

	void App::postInit(CommandList commands)
	{	
		const auto hades_version_major = 0, 
			hades_version_minor = 0,
			hades_version_patch  = 0;

		LOG("Hades " + std::to_string(hades_version_major) + "." + std::to_string(hades_version_minor) + "." + std::to_string(hades_version_patch));
		LOG("SFML " + std::to_string(SFML_VERSION_MAJOR) + "." + std::to_string(SFML_VERSION_MINOR) + "." + std::to_string(SFML_VERSION_PATCH));
		LOG("Thor " + std::to_string(THOR_VERSION_MAJOR) + "." + std::to_string(THOR_VERSION_MINOR));
		LOG("TGUI " + std::to_string(TGUI_VERSION_MAJOR) + "." + std::to_string(TGUI_VERSION_MINOR) + "." + std::to_string(TGUI_VERSION_PATCH));
		LOG("zlib " + types::string(ZLIB_VERSION));
		//yaml-cpp doesn't currently have a version macro
		LOG("yaml-cpp 0.5.3"); //TODO: base this off the version compiled
		//add default game
		commands.push_back("game " + defaultGame());
		//pull -game and -mod commands from commands
		//and load them in the datamanager.
		auto &data = _dataMan;
		loadCommand(commands, "game", [&data](std::string command) {
			std::stringstream params(command);
			std::string value;
			std::getline(params, value, ' ');
			assert(value == "game");

			std::getline(params, command, '\0');

			#ifndef NDEBUG
				//if debug, load mod archives from the asset directory
				command = "../../game/" + command;
			#endif

			data.load_game(command);
		});

		loadCommand(commands, "mod", [&data](std::string command) {
			std::stringstream params(command);
			std::string value;
			std::getline(params, value, ' ');
			assert(value == "mod");

			std::getline(params, command, '\0');

			#ifndef NDEBUG
				//if debug, load mod archives from the asset directory
				command = "../../game/" + command;
			#endif

			data.add_mod(command);
		});

		defaultBindings(_input);

		//proccess config files
		//load saved bindings and whatnot from config files

		//if hades main handles any of the commands then they will be removed from 'commands'
		hadesMain(_states, _console, commands);


		//process command lines
		//pass the commands into the console to be fullfilled using the normal parser

		if (!_console->runCommand("vid_reinit"))
		{
			_console->echo("Error setting video, falling back to default", Console::ERROR);
			_console->runCommand("vid_default");
			_console->runCommand("vid_reinit");
		}
	}

	bool App::run()
	{
		//We copy the famous gaffer on games timestep here.
		//however we don't have a system to blend renderstates,
		//we use curves instead.

		//tickrate is the amount of time simulated at any one tick
		auto tickrate = _console->getValue<int>("c_ticktime"),
			maxframetime  = _console->getValue<int>("c_maxticktime");

		assert(tickrate && "failed to get tick rate value from console");

		sf::Clock time;
		time.restart();

		

		bool running = true;

		//t = total app running time,
		// only counting completed game ticks
		//sf::Time t = sf::Time::Zero;
		sf::Time currentTime = time.getElapsedTime();
		sf::Time accumulator = sf::Time::Zero;

		while(running && _window.isOpen())
		{
			State *activeState = _states.getActiveState();
			if(!activeState)
				break;

			sf::Time dt = sf::milliseconds(*tickrate);

			sf::Time newTime = time.getElapsedTime();
			sf::Time frameTime = newTime - currentTime;

			if (frameTime > sf::milliseconds(*maxframetime))
				frameTime = sf::milliseconds(*maxframetime);

			currentTime = newTime;
			accumulator += frameTime;

			//perform additional logic updates if we're behind on logic

			//this frame the total amount of time this frame has taken.
			sf::Time thisFrame = sf::Time::Zero;

			while( accumulator >= dt)
			{
				auto unhandled = handleEvents(activeState);
				_input.generateState(unhandled);

				activeState->update(dt, _input.getInputState());
				//t += dt;
				accumulator -= dt;
				thisFrame += dt;
			}

			_window.clear();
			//drawing must pass the frame time, so that the renderer can 
			//interpolate between frames
			activeState->draw(_window, thisFrame + accumulator);
			activeState->drawGui();

			//render the console interface if it is active.
			if (_consoleView.active())
			{
				_consoleView.update();
				_consoleView.draw(_window);
			}

			_window.display();
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

		//shutdown the job system
		parallel_jobs::join();

		//unregister the global providers
		console::log = nullptr;
		console::property_provider = nullptr;
		console::system_object = nullptr;
	}

	std::vector<sf::Event> App::handleEvents(State *activeState)
	{
		//drop events from last frame
		//_bindings.dropEvents();

		std::vector<sf::Event> events;

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
			else if (!activeState->handleEvent(e)) // otherwise let the gamestate handle it
				events.push_back(e);				//if the state doesn't handle the event directly
													// then let the binding manager pass it as user input
		}

		return events;
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
			_console->registerFunction("exit", exit, true);
			_console->registerFunction("quit", exit, true);

			//_console->registerFunction("bind", std::bind(&Bind::bindControl, &_bindings, std::placeholders::_1), true);
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

			_console->registerFunction("vid_reinit", vid_reinit, true);
		}
	}
}//hades