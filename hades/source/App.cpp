#include "Hades/App.hpp"

#include <cassert>
#include <string>

#include "SFML/Window/Event.hpp"

#include "hades/console_variables.hpp"
#include "hades/core_resources.hpp"
#include "hades/data.hpp"
#include "hades/debug.hpp"
#include "Hades/fps_display.hpp"
#include "hades/game_loop.hpp"
#include "hades/logging.hpp"
#include "hades/parser.hpp"
#include "hades/properties.hpp"
#include "hades/simple_resources.hpp"
#include "hades/state.hpp"
#include "hades/timers.hpp"
#include "hades/writer.hpp"
#include "hades/yaml_parser.hpp"
#include "hades/yaml_writer.hpp"

#include "hades/debug/console_overlay.hpp"

using namespace std::string_literals;
using namespace std::string_view_literals;

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

	void App::init()
	{
		//record the global console as logger
		console::log = &_console;
		//record the console as the property provider
		console::property_provider = &_console;
		//record the console as the engine command line
		console::system_object = &_console;
		//record the thread pool as the proccess shared pool
		detail::set_shared_thread_pool(&_thread_pool);

		//register sfml input names
		register_sfml_input(_window, _input);

		register_core_resources(_dataMan);
		RegisterCommonResources(&_dataMan);
		data::detail::set_data_manager_ptr(&_dataMan);

		//debug overlays
		_gui.emplace(); // default construct the gui object now that the data_manager is available
		debug::set_overlay_manager(&_overlay_manager);
		_text_overlay_manager.emplace();
		debug::set_text_overlay_manager(&*_text_overlay_manager);
		debug::set_screen_overlay_manager(&_screen_overlay_manager);

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
		return LoadCommand(commands, command, [&job](auto&&)
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

	static void update_overlay_size(sf::View& v, gui& g,
		float w, float h) noexcept
	{
		v.setSize(w, h);
		v.setCenter(w / 2.f, h / 2.f);
		g.set_display_size({ w, h });
		return;
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
		LOG("zlib " + to_string(zip::zlib_version()));
		LOG("yaml-cpp " + to_string(HADES_YAML_VERSION));
		
		//pull -game and -mod commands from commands
		//and load them in the datamanager.
		auto &data = _dataMan;
		auto load_game = [&data](const argument_list &command) {
			if (command.size() != 1)
			{
				LOGERROR("game command expects a single argument"sv);
				return false;
			}

			data.load_game(to_string(command.front()));

			return true;
		};

		if (!LoadCommand(commands, "game"sv, load_game))
		{
			//add default game if one isnt specified
			command com;
			com.request = "game"sv;
			com.arguments.push_back(defaultGame());

			commands.push_back(com);
			LoadCommand(commands, "game"sv, load_game);
		}

		LoadCommand(commands, "mod"sv, [&data](const argument_list &command) {
			if (command.size() != 1)
			{
				LOGERROR("game command expects a single argument"sv);
				return false;
			}

			data.add_mod("./"s + to_string(command.front()));
			return true;
		});

		_window.resetGLStates();

		//if hades main handles any of the commands then they will be removed from 'commands'
		hadesMain(_states, _input, commands);

		//TODO: handle autoexec.console files
		//TODO: handle config.conf settings files
		//proccess config files
		//load saved bindings and whatnot from config files

		//process command lines
		//pass the commands into the console to be fullfilled using the normal parser
		for (auto &c : commands)
			console::run_command(c);

		//create  the normal window
		if (!_console.run_command(command("vid_reinit"sv)))
		{
			LOGERROR("Error setting video, falling back to default"sv);
			_console.run_command(command("vid_default"sv));
			_console.run_command(command("vid_reinit"sv));
		}

		//create the debug view and gui settings
		const auto width = console::get_int(cvars::video_width, cvars::default_value::video_width);
		const auto height = console::get_int(cvars::video_height, cvars::default_value::video_height);
		const auto widthf = static_cast<float>(width->load());
		const auto heightf = static_cast<float>(height->load());
		update_overlay_size(_overlay_view, *_gui, widthf, heightf);
	}

	static void record_tick_stats(const std::vector<time_duration>& times,
		console::property_float avg, console::property_float max, console::property_float min) noexcept
	{
		auto max_t = time_duration::zero();
		auto min_t = time_duration{ seconds{ 500 } };
		auto accumulator = time_duration::zero();

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

		game_loop_timing gl_times;
		performance_statistics game_loop_metrics;

		while(_window.isOpen())
		{
			state *activeState = _states.getActiveState();
			if(!activeState)
				break;

			const auto dt = time_duration{ seconds{ 1 } } / tick_rate->load();

			auto on_tick = [this, dt, state = activeState]() {
				handleEvents(state);
				_input.generate_state(_event_buffer);
				state->update(dt, _window, _input.input_state());
			};

			auto on_draw = [this, state = activeState](time_duration dt) {
				if (!_window.isOpen())
					return;

				_window.clear();
				//drawing must pass the frame time, so that the renderer can
				//interpolate between frames
				state->draw(_window, dt);

				//draw debug overlays
				_screen_overlay_manager.draw(dt, _window);
				_window.setView(_overlay_view);
				_gui->activate_context();
				_gui->update(dt);
				_gui->frame_begin();
				_overlay_manager.update(*_gui);
				if (_show_gui_demo)
					_gui->show_demo_window();
				_gui->frame_end();
				
				_gui->draw(_window);
				_text_overlay_manager->update();
				_text_overlay_manager->draw(dt, _window);

				_window.display();
				return;
			};

			game_loop(gl_times, dt, on_tick, on_draw, game_loop_metrics);

			//TODO: dont use console vars to store this stuff? 
			//tick stats
			record_tick_stats(game_loop_metrics.tick_times, avg_tick_time, max_tick_time, min_tick_time);
			
			//frame time
			using milliseconds_float = basic_duration<float, std::chrono::milliseconds::period>;
			const auto frame_time_ms = time_cast<milliseconds_float>(game_loop_metrics.previous_frame_time);
			last_frame_time->store(frame_time_ms.count());

			//total update time
			const auto update_time = time_cast<milliseconds_float>(game_loop_metrics.update_duration);
			total_tick_time->store(update_time.count());

			//tick count
			frame_tick_count->store(integer_cast<int32>(game_loop_metrics.tick_count));
		
			//drawing time
			const auto float_draw_time = time_cast<milliseconds_float>(game_loop_metrics.draw_duration);
			frame_draw_time->store(float_draw_time.count());
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
		//TODO: what about data_manager
	}

	void App::handleEvents(state *activeState)
	{
		_event_buffer.clear();
		using event = input_event_system::event;
		auto e = event{};
		while (_window.pollEvent(e))
		{
			bool handled = false;
			//window events
			if (e.type == event::Closed) // if the app is closed, then disappear without a fuss
				_window.close();
			else if (e.type == event::KeyPressed && // otherwise check for console summon
				e.key.code == sf::Keyboard::Tilde)
			{
				if (_console_debug)
				{
					_console_debug = static_cast<debug::console_overlay*>
						(debug::destroy_overlay(_console_debug));
				}
				else
				{
					_console_debug = static_cast<debug::console_overlay*>
						(debug::create_overlay(std::make_unique<debug::console_overlay>()));
				}
			}
			else if (e.type == sf::Event::Resized)	// handle resize before _console_debug, so that opening the console
			{									// doesn't block resizing the window
				_console.setValue<types::int32>(cvars::video_width, e.size.width);
				_console.setValue<types::int32>(cvars::video_height, e.size.height);
				const auto widthf = static_cast<float>(e.size.width);
				const auto heightf = static_cast<float>(e.size.height);
				update_overlay_size(_overlay_view, *_gui, widthf, heightf);

				_states.getActiveState()->reinit();
				activeState->handle_event(e); // let the gamestate see the changed window size
			}
			//else if (_console_debug)	// if the console is active forward all input to it rather than the gamestate
			//{
			//	if (e.type == event::KeyPressed)
			//	{
			//		if (e.key.code == sf::Keyboard::Up)
			//			_console_debug->prev();
			//		else if (e.key.code == sf::Keyboard::Down)
			//			_console_debug->next();
			//		else if (e.key.code == sf::Keyboard::Return)
			//			_console_debug->send_command();
			//	}
			//	else if (e.type == event::TextEntered)
			//		_console_debug->enter_text(e);
			//}
			//input events
			else
			{
				_gui->activate_context();
				if (_gui->handle_event(e)) // check debug overlay ui
					handled = true; 
				else if (activeState->handle_event(e)) // check state raw event
					handled = true;

				if(handled == false)
					_event_buffer.emplace_back(input_event_system::checked_event{ handled, e });
			}
		}

		return;
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
			_console.add_function("exit"sv, exit, true);
			_console.add_function("quit"sv, exit, true);
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
					LOGERROR("Attempted to set invalid video mode."sv);
					return false;
				}

				auto window_type = sf::Style::Titlebar | sf::Style::Close;
				
				const auto resizable = _console.getBool(cvars::video_resizable);

				if (fullscreen->load())
					window_type = sf::Style::Fullscreen;
				else if (resizable->load())
					window_type |= sf::Style::Resize;

				_window.create(mode, "game", window_type);
				
				//restore vsync settings
				_window.setFramerateLimit(0);
				_window.setVerticalSyncEnabled(_framelimit.vsync);

				_states.getActiveState()->reinit();

				return true;
			};

			_console.add_function("vid_reinit"sv, vid_reinit, true);

			auto vsync = [this](const argument_list &args)->bool {
				if (args.size() != 1)
				{
					LOG("vid_vsync 0"sv);
					return true;
				}

				try
				{
					_framelimit.vsync = args.front() == "true"sv || std::stoi(to_string(args.front())) > 0;
					_window.setFramerateLimit(0);
					LOG("vid_vsync " + to_string(args.front()));
				}
				catch (std::invalid_argument&)
				{
					LOGERROR("vid_vsync "s + to_string(args.front()));
					LOGERROR("Unable to parse value: '"s + to_string(args.front()) + "'"s);
					return false;
				}
				catch (std::out_of_range&)
				{
					LOGERROR("vid_vsync "s + to_string(args.front()));
					LOGERROR("Invalid value: '"s + to_string(args.front()) + "'"s);
					return false;
				}

				_window.setVerticalSyncEnabled(_framelimit.vsync);
				return true;
			};

			_console.add_function("vid_vsync"sv, vsync, true, true);

			auto framelimit = [this](const argument_list &args)->bool {
				if (args.size() != 1)
				{
					if (_framelimit.vsync)
						LOG("vid_framelimit 0"sv);
					else
						LOG("vid_framelimit "s + to_string(_framelimit.framelimit));
					return true;
				}

				if (_framelimit.vsync)
				{
					_framelimit.vsync = false;
					_window.setVerticalSyncEnabled(_framelimit.vsync);
				}

				try
				{
					const auto limit = std::stoi(to_string(args.front()));
					_window.setFramerateLimit(limit);
				}
				catch (std::invalid_argument&)
				{
					LOGERROR("vid_framelimit "s + to_string(args.front()));
					LOGERROR("Invalid value for vid_framelimit setting to 0"sv);
					_window.setFramerateLimit(0);
					return false;
				}
				catch (std::out_of_range&)
				{
					LOGERROR("vid_framelimit "s + to_string(args.front()));
					LOGERROR("Invalid value for vid_framelimit setting to 0"sv);
					_window.setFramerateLimit(0);
					return false;
				}

				return true;
			};

			_console.add_function("vid_framelimit"sv, framelimit, true, true);
		}
		//Filesystem functions
		{
			auto util_dir = [this](const argument_list &args)->bool {
				if (args.size() != 1)
					throw invalid_argument("Dir function expects one argument"s);

				const auto files = files::ListFilesInDirectory(to_string(args.front()));

				for (auto f : files)
					LOG(f);

				return true;
			};

			_console.add_function("dir", util_dir, true);

			auto compress_dir = [](const argument_list &args)->bool
			{
				if (args.size() != 1)
					throw invalid_argument("Compress function expects one argument"s);

				const auto& path = args.front();

				//compress in a seperate thread to let the UI continue updating
				std::thread t([path]() {
					try {
						zip::compress_directory(to_string(path));
						return true;
					}
					catch (files::file_error &e)
					{
						LOGERROR(e.what());
					}

					return false;
				});

				t.detach();

				return true;
			};

			_console.add_function("compress"sv, compress_dir, true);

			auto uncompress_dir = [](const argument_list &args)->bool
			{
				if (args.size() != 1)
					throw invalid_argument("Uncompress function expects one argument"s);

				const auto& path = args.front();

				//run uncompress func in a seperate thread to spare the UI
				std::thread t([path]() {
					try {
						zip::uncompress_archive(to_string(path));
						return true;
					}
					catch (files::file_error &e)
					{
						LOGERROR(e.what());
					}

					return false;
				});

				t.detach();

				return true;
			};

			_console.add_function("uncompress"sv, uncompress_dir, true);
		}

		//debug funcs
		{
			//stats display
			auto stats = [](const argument_list &args) {
				//TODO: why would this throw
				if (args.size() != 1)
					throw invalid_argument("fps function expects one argument"s);

				const auto param = args[0];
				const auto int_param = from_string<int32>(param);
				create_fps_overlay(int_param);

				return true;
			};

			_console.add_function("stats"sv, stats, true);

			//start the stats by default on debug builds
			#ifndef NDEBUG
			stats({ "1"sv });
			#endif

			auto imgui_demo = [&g = _show_gui_demo]() {
				g = !g;
				return true;
			};

			_console.add_function("imgui_demo"sv, imgui_demo, true);
		}
	}
}//hades
