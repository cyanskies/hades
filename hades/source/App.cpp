#include "Hades/App.hpp"

#include <cassert>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <sstream>

#include "SFML/System/Clock.hpp"
#include "SFML/Window/Event.hpp"

#include "zlib/zlib.h" //for zlib version

#include "Hades/ConsoleView.hpp"
#include "Hades/Data.hpp"
#include "Hades/Debug.hpp"
#include "Hades/files.hpp"
#include "Hades/Logging.hpp"
#include "Hades/parallel_jobs.hpp"
#include "Hades/Properties.hpp"
#include "Hades/simple_resources.hpp"
#include "Gui/Gui.hpp"

namespace hades
{
	//true float values for common ratios * 100
	const int RATIO3_4 = 133, RATIO16_9 = 178, RATIO16_10 = 160;

	//console variable names
	//c_* client variables names
	// for variables that control basic client behaviour: framerate, etc
	//con_* console variable names
	// for console behaviour, font type, size and so on
	//r_* render variable names
	// none currently?
	//s_* server variables
	// server framerate and so on
	//vid_* video settings variables
	// resolution, colour depth, fullscreen, etc
	//file_* for file path settings and read/write settings
	//a_* for audio settings
	//n_* for network settings

	void registerVariables(Console *console)
	{
		//console variables
		console->set("con_charactersize", 15);
		console->set("con_fade", 180);

		//app variables
		console->set("c_ticktime", 30);
		console->set("c_maxticktime", 150); // 1.5 seconds is the maximum allowable tick time.
		console->set("file_portable", false); // activates portable mode, where files are only read/writen to the game directory
	}

	void registerVidVariables(Console *console)
	{
		std::function<bool(std::string)> vid_default = [console] (std::string)->bool {
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

	void registerServerVariables(Console *console)
	{
		console->set("s_tickrate", 30);
		console->set("s_maxframetime", 150); // 1.5 seconds is the maximum allowable frame time.
		console->set("s_connectionport", 0); // 0 = auto port
		console->set("s_portrange", 0); // unused
	}

	App::App() : _consoleView(nullptr), _input(_window), _sfVSync(false)
	{}

	void App::init()
	{
		//record the global console as logger
		console::log = &_console;
		//record the console as the property provider
		console::property_provider = &_console;
		//record the console as the engine command line
		console::system_object = &_console;

		debug::overlay_manager = &_overlayMan;

		RegisterCommonResources(&_dataMan);
		data::detail::SetDataManagerPtr(&_dataMan);

		resourceTypes(_dataMan);

		//register custom TGUI widgets
		gui::RegisterGuiElements();

		//load defualt console settings
		registerVariables(&_console);
		registerVidVariables(&_console);
		registerServerVariables(&_console);

		//load config files and overwrite any updated settings
		_states.setGuiTarget(_window);

		registerConsoleCommands();

		//initialise the job system
		parallel_jobs::init();
	}

	//calls job on any command that contains command
	bool LoadCommand(CommandList &commands, types::string command, std::function<void(CommandList::value_type)> job)
	{
		//FIXME: why is this called bit?
		auto bit = commands.begin();
		bool ret = false;
		while (bit != std::end(commands))
		{
			auto com = bit->substr(0, command.length());

			if (com == command)
			{
				std::string params;
				//replace with std::move
				std::copy(bit->begin() + command.length() + 1, bit->end(), std::back_inserter(params));
				job(params);
				bit = commands.erase(bit);
				ret = true;
			}
			else
				++bit;
		}

		return ret;
	}

	void App::postInit(CommandList commands)
	{
		const auto hades_version_major = 0,
			hades_version_minor = 1,
			hades_version_patch  = 0;

		//create a hidden window early to let us start making textures without
		//creating GL errors
		//this will be replaced with the proper window after the mods are loaded.
		_window.create(sf::VideoMode(), "hades", sf::Style::None);

		LOG("Hades " + std::to_string(hades_version_major) + "." + std::to_string(hades_version_minor) + "." + std::to_string(hades_version_patch));
		LOG("SFML " + std::to_string(SFML_VERSION_MAJOR) + "." + std::to_string(SFML_VERSION_MINOR) + "." + std::to_string(SFML_VERSION_PATCH));
		LOG("TGUI " + std::to_string(TGUI_VERSION_MAJOR) + "." + std::to_string(TGUI_VERSION_MINOR) + "." + std::to_string(TGUI_VERSION_PATCH));
		LOG("zlib " + types::string(ZLIB_VERSION));
		//yaml-cpp doesn't currently have a version macro
		LOG("yaml-cpp 0.5.3"); //TODO: base this off the version compiled
		//add default game
		commands.push_back("game " + defaultGame());
		//pull -game and -mod commands from commands
		//and load them in the datamanager.
		auto &data = _dataMan;
		LoadCommand(commands, "game", [&data](std::string command) {
			data.load_game("./" + command);
		});

		LoadCommand(commands, "mod", [&data](std::string command) {
			data.add_mod("./" + command);
		});

		//if hades main handles any of the commands then they will be removed from 'commands'
		hadesMain(_states, _input, commands);

		//TODO: handle autoconsole files
		//TODO: handle config settings files
		//proccess config files
		//load saved bindings and whatnot from config files

		//process command lines
		//pass the commands into the console to be fullfilled using the normal parser
		for (auto &c : commands)
			console::runCommand(c);

		//create  the normal window
		if (!_console.runCommand("vid_reinit"))
		{
			_console.echo("Error setting video, falling back to default", Console::ERROR);
			_console.runCommand("vid_default");
			_console.runCommand("vid_reinit");
		}
	}

	bool App::run()
	{
		//We copy the famous gaffer on games timestep here.
		//however we don't have a system to blend renderstates,
		//we use curves instead.

		//tickrate is the amount of time simulated at any one tick
		auto tickrate = _console.getValue<types::int32>("c_ticktime"),
			maxframetime  = _console.getValue<types::int32>("c_maxticktime");

		assert(tickrate && "failed to get tick rate value from console");

		sf::Clock time;
		time.restart();

		bool running = true;

		//currentTime is the total running time of the application
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

			//the total amount of time this frame has taken.
			sf::Time thisFrame = sf::Time::Zero;

			//perform additional logic updates if we're behind on logic
			while( accumulator >= dt)
			{
				auto events = handleEvents(activeState);
				_input.generateState(events);

				activeState->update(dt, _window, _input.getInputState());
				accumulator -= dt;
				thisFrame += dt;
			}

			_window.clear();
			//drawing must pass the frame time, so that the renderer can
			//interpolate between frames
			activeState->draw(_window, thisFrame + accumulator);
			activeState->drawGui();

			//render the console interface if it is active.
			if (_consoleView)
				_consoleView->update();

			_window.draw(_overlayMan);

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
		debug::overlay_manager = nullptr;
	}

	std::vector<Event> App::handleEvents(State *activeState)
	{
		std::vector<Event> events;

		sf::Event e;
		while (_window.pollEvent(e))
		{
			bool handled = false;
			//window events
			if (e.type == sf::Event::Closed) // if the app is closed, then disappear without a fuss
				_window.close();
			else if (e.type == sf::Event::KeyPressed && // otherwise check for console summon
				e.key.code == sf::Keyboard::Tilde)
			{
				if (_consoleView)
					_consoleView = static_cast<ConsoleView*>(debug::DestroyOverlay(_consoleView));
				else
					_consoleView = static_cast<ConsoleView*>(debug::CreateOverlay(std::make_unique<ConsoleView>()));
			}
			else if (e.type == sf::Event::Resized)	// handle resize before _consoleView, so that opening the console
			{									// doesn't block resizing the window
				_console.set<types::int32>("vid_width", e.size.width);
				_console.set<types::int32>("vid_height", e.size.height);
				_console.set("vid_mode", -1);

				_overlayMan.setWindowSize({ e.size.width, e.size.height });
				_states.getActiveState()->reinit();
				_states.getActiveState()->setGuiView(sf::View({0.f , 0.f, static_cast<float>(e.size.width), static_cast<float>(e.size.height)}));
				activeState->handleEvent(std::make_tuple(true, e));		// let the gamestate see the changed window size
			}
			else if (_consoleView)	// if the console is active forward all input to it rather than the gamestate
			{
				if (e.type == sf::Event::KeyPressed)
				{
					if (e.key.code == sf::Keyboard::Up)
						_consoleView->prev();
					else if (e.key.code == sf::Keyboard::Down)
						_consoleView->next();
					else if (e.key.code == sf::Keyboard::Return)
						_consoleView->sendCommand();
				}
				else if (e.type == sf::Event::TextEntered)
					_consoleView->enterText(e);
			}
			//input events
			else
			{
				if (activeState->guiInput(e))
					handled = true;
				if (activeState->handleEvent(std::make_tuple(handled, e)))
					handled = true;

				events.push_back(std::make_tuple(handled, e));
			}
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
			_console.registerFunction("exit", exit, true);
			_console.registerFunction("quit", exit, true);
		}

		//vid functions
		{
			std::function<bool(std::string)> vid_reinit = [this](std::string)->bool {
				auto width = _console.getValue<types::int32>("vid_width"),
					height = _console.getValue<types::int32>("vid_height"),
					depth = _console.getValue<types::int32>("vid_depth");

				auto fullscreen = _console.getValue<bool>("vid_fullscreen");

				sf::VideoMode mode(width->load(), height->load(), depth->load());

				// if we're in fullscreen mode, then the videomode must be 'valid'
				if (fullscreen->load() && !mode.isValid())
				{
					_console.echo("Attempted to set invalid video mode.", Console::ERROR);
					return false;
				}

				float ratio = static_cast<float>(width->load()) / static_cast<float>(height->load());
				int intratio = static_cast<int>(ratio * 100);

				switch (intratio)
				{
				case RATIO3_4:
					_console.set("vid_mode", 0);
					break;
				case RATIO16_9:
					_console.set("vid_mode", 1);
					break;
				case RATIO16_10:
					_console.set("vid_mode", 2);
					break;
				default:
					_console.set("vid_mode", -1);
					_console.echo("Cannot determine video ratio.", Console::WARNING);
					break;
				}

				if (fullscreen->load())
					_window.create(mode, "game", sf::Style::Fullscreen);
				else
					_window.create(mode, "game", sf::Style::Titlebar | sf::Style::Close | sf::Style::Resize);

				_overlayMan.setWindowSize({ mode.width, mode.height });

				//TODO: restore vsync/framelimit settings to window

				return true;
			};

			_console.registerFunction("vid_reinit", vid_reinit, true);

			auto vsync = [this](std::string value)->bool {
				_window.setFramerateLimit(0);

				try
				{
					_sfVSync = value == "true" || std::stoi(value) > 0;
				}
				catch (std::invalid_argument&)
				{
					_sfVSync = false;
				}
				catch (std::out_of_range&)
				{
					_sfVSync = false;
				}

				_window.setVerticalSyncEnabled(_sfVSync);
				return true;
			};

			_console.registerFunction("vid_vsync", vsync, true);

			auto framelimit = [this](std::string limitstr)->bool {
				if (_sfVSync)
				{
					_sfVSync = false;
					_window.setVerticalSyncEnabled(_sfVSync);
				}

				try
				{
					auto limit = std::stoi(limitstr);
					_window.setFramerateLimit(limit);
				}
				catch (std::invalid_argument&)
				{
					LOGWARNING("Invalid value for vid_framelimit setting to 0");
					_window.setFramerateLimit(0);
				}
				catch (std::out_of_range&)
				{
					LOGWARNING("Invalid value for vid_framelimit setting to 0");
					_window.setFramerateLimit(0);
				}

				return true;
			};

			_console.registerFunction("vid_framelimit", framelimit, true);
		}
		//Filesystem functions
		{
			std::function<bool(std::string)> util_dir = [this](std::string path)->bool {
				auto files = files::ListFilesInDirectory(path);

				for (auto f : files)
					LOG(f);

				return true;
			};

			_console.registerFunction("dir", util_dir, true);

			std::function<bool(types::string)> compress_dir = [](types::string path)->bool
			{
				//compress in a seperate thread to let the UI continue updating
				std::thread t([path]() {
					try {
						zip::compress_directory(path);
						return true;
					}
					catch (zip::archive_exception &e)
					{
						LOGERROR(e.what());
					}

					return false;
				});

				t.detach();

				return true;
			};

			_console.registerFunction("compress", compress_dir, true);

			std::function<bool(types::string)> uncompress_dir = [](types::string path)->bool
			{
				//run uncompress func in a seperate thread to spare the UI
				std::thread t([path]() {
					try {
						zip::uncompress_archive(path);
						return true;
					}
					catch (zip::archive_exception &e)
					{
						LOGERROR(e.what());
					}

					return false;
				});

				t.detach();

				return true;
			};

			_console.registerFunction("uncompress", uncompress_dir, true);
		}
	}
}//hades
