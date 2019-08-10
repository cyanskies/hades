#include "Hades/App.hpp"

#include <cassert>
#include <string>

#include "SFML/Window/Event.hpp"

#include "zlib.h" //for zlib version

#include "Hades/ConsoleView.hpp"
#include "hades/core_resources.hpp"
#include "Hades/Data.hpp"
#include "Hades/Debug.hpp"
#include "Hades/fps_display.hpp"
#include "Hades/Logging.hpp"
#include "hades/parser.hpp"
#include "Hades/Properties.hpp"
#include "hades/simple_resources.hpp"
#include "hades/timers.hpp"
#include "hades/writer.hpp"
#include "hades/yaml_parser.hpp"
#include "hades/yaml_writer.hpp"

#include "hades/console_variables.hpp"

namespace hades
{
	//true float values for common ratios * 100
	constexpr int RATIO3_4 = 133, RATIO16_9 = 178, RATIO16_10 = 160;

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

	App::App() : _consoleView{ nullptr }, _sfVSync{ false }
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

		data::set_default_parser(data::make_yaml_parser);
		data::set_default_writer(data::make_yaml_writer);
		
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
				ret = job(com_iter->arguments);
				com_iter = commands.erase(com_iter);
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
		//LOG("SFGUI " + std::to_string(SFGUI_MAJOR_VERSION) + "." + std::to_string(SFGUI_MINOR_VERSION) + "." + std::to_string(SFGUI_REVISION_VERSION));
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

	static void record_tick_stats(const std::vector<time_duration>& times,
		console::property_float avg, console::property_float max, console::property_float min) noexcept
	{
		auto max_t = time_duration{};
		auto min_t = time_duration{ seconds{500} };
		auto accumulator = time_duration{};

		for(const auto &t : times)
		{
			max_t = std::max(t, max_t);
			min_t = std::min(t, min_t);
			accumulator += t;
		}

		const auto count = std::size(times);

		if (count == 0)
		{
			avg->store(0.f);
			min->store(0.f);
			max->store(0.f);
			return;
		}

		const auto max_f = time_cast<milliseconds_float>(max_t);
		const auto min_f = time_cast<milliseconds_float>(min_t);
		const auto avg_f = time_cast<milliseconds_float>(accumulator / count);
		avg->store(avg_f.count());
		min->store(min_f.count());
		max->store(max_f.count());
		return;
	}

	bool App::run()
	{
		//We copy the famous gaffer on games timestep here.
		//however we don't have a system to blend renderstates,
		//we use curves instead.

		//TODO: use safe functions for this from the console:: namespace
		//ticks per second, dt = 1 / tick_rate
		const auto tick_rate = _console.getInt(cvars::client_tick_rate);
		//const auto maxframetime = _console.getInt(cvars::client_max_tick); // NOTE: unused
		auto avg_tick_time = _console.getFloat(cvars::client_avg_tick_time);
		auto max_tick_time = _console.getFloat(cvars::client_max_tick_time);
		auto min_tick_time = _console.getFloat(cvars::client_min_tick_time);
		auto total_tick_time = _console.getFloat(cvars::client_total_tick_time);
		auto last_frame_time = _console.getFloat(cvars::client_previous_frametime);
		auto frame_tick_count = _console.getInt(cvars::client_tick_count);
		auto frame_draw_time = _console.getFloat(cvars::render_drawtime);

		assert(tick_rate && "failed to get tick rate value from console");

		//auto t = time_duration{}; // < total accumulated time of application
		auto current_time = time_clock::now();
		auto accumulator = time_duration{};

		int32 frame_ticks = 1;
		auto prev_tick_times = std::vector<time_duration>{};
		bool running = true;

		while(running && _window.isOpen())
		{
			state *activeState = _states.getActiveState();
			if(!activeState)
				break;

			const auto dt = time_duration{ seconds{ 1 } } / tick_rate->load();

			const auto new_time = time_clock::now();
			auto frame_time = new_time - current_time;

			//store framerate
			{
				using milliseconds_float = basic_duration<float, std::chrono::milliseconds::period>;
				const auto frame_time_ms = time_cast<milliseconds_float>(frame_time);
				last_frame_time->store(frame_time_ms.count());
			}

			constexpr auto max_accumulator_overflow = time_cast<time_duration>(seconds_float{ 0.25f });
			if (frame_time > max_accumulator_overflow)
				frame_time = max_accumulator_overflow;

			current_time = new_time;
			accumulator += frame_time;

			prev_tick_times.clear();

			const auto update_start = time_clock::now();

			//perform additional logic updates if we're behind on logic
			while(accumulator >= dt)
			{
				const auto tick_start = time_clock::now();
				auto events = handleEvents(activeState);
				_input.generate_state(events);

				activeState->update(dt, _window, _input.input_state());
				//t += dt;
				accumulator -= dt;
				++frame_ticks;
				prev_tick_times.emplace_back(time_clock::now() - tick_start);
			}

			const auto update_time = time_cast<milliseconds_float>(time_clock::now() - update_start);
			total_tick_time->store(update_time.count());

			record_tick_stats(prev_tick_times, avg_tick_time, max_tick_time, min_tick_time);

			frame_tick_count->store(frame_ticks);
			frame_ticks = 1;

			if (_window.isOpen())
			{
				const auto draw_start = time_clock::now();

				_window.clear();
				//drawing must pass the frame time, so that the renderer can
				//interpolate between frames
				activeState->draw(_window, frame_time);

				//update the console interface.
				if (_consoleView)
					_consoleView->update();

				//draw overlays
				_overlayMan.draw(frame_time, _window);
				
				_window.display();

				const auto total_draw_time = time_clock::now() - draw_start;
				const auto float_draw_time = time_cast<milliseconds_float>(total_draw_time);
				frame_draw_time->store(float_draw_time.count());
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

	std::vector<input_event_system::checked_event> App::handleEvents(state *activeState)
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
				activeState->handle_event(e);		// let the gamestate see the changed window size
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
				if (activeState->handle_event(e))
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

				//TODO: reinit active state

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

		//debug funcs
		{
			//fps display
			auto fps = [](const argument_list &args) {
				if (args.size() != 1)
					throw invalid_argument("fps function expects one argument");

				const auto param = args[0];
				const auto int_param = from_string<int32>(param);
				create_fps_overlay(int_param);

				return true;
			};

			_console.add_function("show_fps", fps, true);

			#ifndef NDEBUG
				fps({ "1" });
			#endif
		}
	}
}//hades
