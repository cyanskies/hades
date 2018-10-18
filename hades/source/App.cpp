#include "Hades/App.hpp"

#include <cassert>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <sstream>

#include "SFML/System/Clock.hpp"
#include "SFML/Window/Event.hpp"

#include "zlib.h" //for zlib version

#include "Hades/ConsoleView.hpp"
#include "hades/core_resources.hpp"
#include "Hades/Data.hpp"
#include "Hades/Debug.hpp"
#include "Hades/files.hpp"
#include "Hades/Logging.hpp"
#include "Hades/parallel_jobs.hpp"
#include "Hades/Properties.hpp"
#include "Hades/simple_resources.hpp"

#include "hades/console_variables.hpp"

namespace hades
{
	//true float values for common ratios * 100
	constexpr int RATIO3_4 = 133, RATIO16_9 = 178, RATIO16_10 = 160;

	void registerVariables(Console *console)
	{
		//console variables
		//console::set_property(console_character_size, console_character_size_d);
		//console::set_property(console_fade, console_fade_d);

		//app variables
		//console::set_property(client_tick_time, client_tick_time_d);
		//console::set_property(client_max_tick, client_max_tick_d); // 1.5 seconds is the maximum allowable tick time.
		// activates portable mode, where files are only read/writen to the game directory
		//console::set_property(file_portable, file_portable_d);
	}

	void registerVidVariables(Console *console)
	{
		auto vid_default = [console] ()->bool {
			//window
			console->set(cvars::video_fullscreen, cvars::default_value::video_fullscreen);
			console->set(cvars::video_resizable, cvars::default_value::video_resizable);
			//resolution
			console->set(cvars::video_width, cvars::default_value::video_width);
			console->set(cvars::video_height, cvars::default_value::video_height);
			//depth
			console->set(cvars::video_depth, cvars::default_value::video_depth);

			return true;
		};

		
		//set the default now
		vid_default();

		//add a command so that this function can be called again
		console->add_function("vid_default", vid_default, true);
	}

	void registerServerVariables(Console *console)
	{
		//console->set("s_tickrate", 30);
		//console->set("s_maxframetime", 150); // 1.5 seconds is the maximum allowable frame time.
		//console->set("s_connectionport", 0); // 0 = auto port
		//console->set("s_portrange", 0); // unused
	}

	App::App() : _consoleView(nullptr), _sfVSync(false)
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

		//register sfml input names
		register_sfml_input(_window, _input);

		register_core_resources(_dataMan);
		RegisterCommonResources(&_dataMan);
		data::detail::set_data_manager_ptr(&_dataMan);

		resourceTypes(_dataMan);

		//load defualt console settings
		create_core_console_variables();
		
		//load config files and overwrite any updated settings

		registerConsoleCommands();
	}

	bool LoadCommand(command_list &commands, std::string_view command, console::function_no_argument job)
	{
		return LoadCommand(commands, command, [&job](auto &&args)
		{
			return job();
		});
	}

	//calls job on any command that contains command
	bool LoadCommand(command_list &commands, std::string_view command, console::function job)
	{
		auto com_iter = commands.begin();
		bool ret = false;
		while (com_iter != std::end(commands))
		{
			if (com_iter->request == command)
			{
				job(com_iter->arguments);
				com_iter = commands.erase(com_iter);
				ret = true;
			}
			else
				++com_iter;
		}

		return ret;
	}

	void App::postInit(command_list commands)
	{
		constexpr auto hades_version_major = 0,
			hades_version_minor = 1,
			hades_version_patch  = 0;

		//create a hidden window early to let us start making textures without
		//creating GL errors
		//this will be replaced with the proper window after the mods are loaded.
		_window.create(sf::VideoMode(), "hades", sf::Style::None);

		LOG("Hades " + std::to_string(hades_version_major) + "." + std::to_string(hades_version_minor) + "." + std::to_string(hades_version_patch));
		LOG("SFML " + std::to_string(SFML_VERSION_MAJOR) + "." + std::to_string(SFML_VERSION_MINOR) + "." + std::to_string(SFML_VERSION_PATCH));
		LOG("SFGUI " + std::to_string(SFGUI_MAJOR_VERSION) + "." + std::to_string(SFGUI_MINOR_VERSION) + "." + std::to_string(SFGUI_REVISION_VERSION));
		LOG("zlib " + types::string(ZLIB_VERSION));
		//yaml-cpp doesn't currently have a version macro
		LOG("yaml-cpp 0.5.3"); //TODO: base this off the version compiled
		
		//pull -game and -mod commands from commands
		//and load them in the datamanager.
		auto &data = _dataMan;
		auto load_game = [&data](const argument_list &command) {
			if (command.size() != 1)
			{
				LOGERROR("game command expects a single argument");
				return false;
			}

			data.load_game("./" + to_string(command.front()));

			return true;
		};

		if (!LoadCommand(commands, "game", load_game))
		{
			//add default game if one isnt specified
			command com;
			com.request = "game";
			com.arguments.push_back(defaultGame());

			commands.push_back(com);
			LoadCommand(commands, "game", load_game);
		}

		LoadCommand(commands, "mod", [&data](const argument_list &command) {
			if (command.size() != 1)
			{
				LOGERROR("game command expects a single argument");
				return false;
			}

			data.add_mod("./" + to_string(command.front()));
			return true;
		});

		_window.resetGLStates();

		//if hades main handles any of the commands then they will be removed from 'commands'
		hadesMain(_states, _input, commands);

		//TODO: handle autoconsole files
		//TODO: handle config settings files
		//proccess config files
		//load saved bindings and whatnot from config files

		//process command lines
		//pass the commands into the console to be fullfilled using the normal parser
		for (auto &c : commands)
			console::run_command(c);

		//create  the normal window
		if (!_console.run_command(command("vid_reinit")))
		{
			LOGERROR("Error setting video, falling back to default");
			_console.run_command(command("vid_default"));
			_console.run_command(command("vid_reinit"));
		}
	}

	bool App::run()
	{
		//We copy the famous gaffer on games timestep here.
		//however we don't have a system to blend renderstates,
		//we use curves instead.

		//tickrate is the amount of time simulated at any one tick
		const auto tickrate = _console.getValue<types::int32>(cvars::client_tick_time),
			maxframetime  = _console.getValue<types::int32>(cvars::client_max_tick);

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

			const sf::Time dt = sf::milliseconds(*tickrate);

			const sf::Time newTime = time.getElapsedTime();
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
				_input.generate_state(events);

				activeState->update(dt, _window, _input.input_state());
				accumulator -= dt;
				thisFrame += dt;
			}

			if (_window.isOpen())
			{
				_window.clear();
				//drawing must pass the frame time, so that the renderer can
				//interpolate between frames
				const auto totalFrameTime = thisFrame + accumulator;
				activeState->draw(_window, totalFrameTime);

				////store gl states while drawing sfgui elements
				//_window.pushGLStates();
				//activeState->updateGui(totalFrameTime);
				//_sfgui.Display(_window);
				//_window.popGLStates(); // restore sfml gl states

				//activeState->drawAfterGui(_window, totalFrameTime);
				//update the console interface.
				if (_consoleView)
					_consoleView->update();

				//draw overlays
				_window.draw(_overlayMan);

				_window.display();
			}
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

		//unregister the global providers
		console::log = nullptr;
		console::property_provider = nullptr;
		console::system_object = nullptr;
		debug::overlay_manager = nullptr;
		//TODO: what about data_manager
		//how about auto unregister from destructor?
	}

	std::vector<input_event_system::checked_event> App::handleEvents(State *activeState)
	{
		std::vector<input_event_system::checked_event> events;

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
				_console.setValue<types::int32>(cvars::video_width, e.size.width);
				_console.setValue<types::int32>(cvars::video_height, e.size.height);

				_overlayMan.setWindowSize({ e.size.width, e.size.height });
				_states.getActiveState()->reinit();
				activeState->handleEvent(e);		// let the gamestate see the changed window size
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
				if (activeState->handleEvent(e))
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
			auto exit = [this]()->bool {
				_window.close();
				return true;
			};
			//exit and quit allow states, players or scripts to close the engine.
			_console.add_function("exit", exit, true);
			_console.add_function("quit", exit, true);
		}

		//vid functions
		{
			auto vid_reinit = [this]()->bool {
				const auto width = _console.getValue<types::int32>(cvars::video_width),
					height = _console.getValue<types::int32>(cvars::video_height),
					depth = _console.getValue<types::int32>(cvars::video_depth);

				const auto fullscreen = _console.getValue<bool>(cvars::video_fullscreen);

				sf::VideoMode mode(width->load(), height->load(), depth->load());

				// if we're in fullscreen mode, then the videomode must be 'valid'
				if (fullscreen->load() && !mode.isValid())
				{
					LOGERROR("Attempted to set invalid video mode.");
					return false;
				}

				auto window_type = sf::Style::Titlebar | sf::Style::Close;
				
				const auto resizable = _console.getBool(cvars::video_resizable);

				if (fullscreen->load())
					window_type = sf::Style::Fullscreen;
				else if (resizable->load())
					window_type |= sf::Style::Resize;

				_window.create(mode, "game", window_type);
				_overlayMan.setWindowSize({ mode.width, mode.height });

				//restore vsync settings
				_window.setFramerateLimit(0);
				_window.setVerticalSyncEnabled(_sfVSync);

				return true;
			};

			_console.add_function("vid_reinit", vid_reinit, true);

			auto vsync = [this](const argument_list &args)->bool {
				if (args.size() != 1)
					throw invalid_argument("vsync function expects one argument");

				_window.setFramerateLimit(0);

				try
				{
					_sfVSync = args.front() == "true" || std::stoi(to_string(args.front())) > 0;
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

			_console.add_function("vid_vsync", vsync, true);

			auto framelimit = [this](const argument_list &args)->bool {
				if (args.size() != 1)
					throw invalid_argument("framelimit function expects one argument");

				if (_sfVSync)
				{
					_sfVSync = false;
					_window.setVerticalSyncEnabled(_sfVSync);
				}

				try
				{
					auto limit = std::stoi(to_string(args.front()));
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

			_console.add_function("vid_framelimit", framelimit, true);
		}
		//Filesystem functions
		{
			auto util_dir = [this](const argument_list &args)->bool {
				if (args.size() != 1)
					throw invalid_argument("Dir function expects one argument");

				const auto files = files::ListFilesInDirectory(to_string(args.front()));

				for (auto f : files)
					LOG(f);

				return true;
			};

			_console.add_function("dir", util_dir, true);

			auto compress_dir = [](const argument_list &args)->bool
			{
				if (args.size() != 1)
					throw invalid_argument("Compress function expects one argument");

				const auto path = args.front();

				//compress in a seperate thread to let the UI continue updating
				std::thread t([path]() {
					try {
						zip::compress_directory(to_string(path));
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

			_console.add_function("compress", compress_dir, true);

			auto uncompress_dir = [](const argument_list &args)->bool
			{
				if (args.size() != 1)
					throw invalid_argument("Uncompress function expects one argument");

				const auto path = args.front();

				//run uncompress func in a seperate thread to spare the UI
				std::thread t([path]() {
					try {
						zip::uncompress_archive(to_string(path));
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

			_console.add_function("uncompress", uncompress_dir, true);
		}
	}
}//hades
