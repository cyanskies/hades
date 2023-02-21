#ifndef HADES_APP_HPP
#define HADES_APP_HPP

#include <optional>
#include <string>
#include <vector>

#include "SFML/Graphics/RenderWindow.hpp"

#include "hades/async.hpp"
#include "hades/Console.hpp"
#include "hades/data_system.hpp"
#include "hades/debug.hpp"
#include "hades/gui.hpp"
#include "hades/sf_input.hpp"
#include "hades/Main.hpp"
#include "hades/StateManager.hpp"
#include "hades/system.hpp"

// Toggles using quick exit to shutdown
#if 1
#define HADES_QUICK_EXIT
#endif

namespace hades::debug
{
	class console_overlay;
}

namespace hades
{
	////////////////////////////////////////////////////////////
	/// \brief Main application class.
	///
	////////////////////////////////////////////////////////////
	class App
	{
	public: 
		// TODO: perform init step in constructor and deinit in destructor

		////////////////////////////////////////////////////////////
		/// \brief Creates and starts most of the apps subsystems and config files.
		///
		////////////////////////////////////////////////////////////
		void init(std::string_view game, register_resource_types_fn);

		////////////////////////////////////////////////////////////
		/// \brief Runs the last few init steps after letting command line variables run.
		///
		///	\param commands The command line arguments.
		///
		////////////////////////////////////////////////////////////
		void postInit(command_list commands, app_main_fn);

		////////////////////////////////////////////////////////////
		/// \brief Starts the main app loop.
		///
		////////////////////////////////////////////////////////////
#ifdef HADES_QUICK_EXIT
		[[noreturn]] 
#endif
		void run();

		////////////////////////////////////////////////////////////
		/// \brief Closes the Window and ends any lingering states.
		///
		////////////////////////////////////////////////////////////
		void cleanUp();
	private:

		////////////////////////////////////////////////////////////
		/// \brief Gives a state the oppurtunity to handle application events.
		///
		///	\param activeState The state which will attempt to handle events.
		///
		/// \return A list of events, if the first value is true, then the event 
		///  has already been handled elsewhere
		///
		////////////////////////////////////////////////////////////
		void handleEvents(state *activeState);

		////////////////////////////////////////////////////////////
		/// \brief Registers the engine provided console commands.
		///
		////////////////////////////////////////////////////////////
		void registerConsoleCommands();
		/// @brief closes the debug console
		void _close_console();

		////////////////////////////////////////////////////////////
		/// \brief Triggers the app to close down.
		///
		////////////////////////////////////////////////////////////
#ifdef HADES_QUICK_EXIT
		[[noreturn]]
#endif
		void _shutdown();

		////////////////////////////////////////////////////////////
		/// Member Data
		////////////////////////////////////////////////////////////
		input_event_system _input;							///< Used by the console to provide bindable input.
		std::vector<input_event_system::checked_event> _event_buffer; ///< Buffer for per-tick event storage
		Console _console;									///< The appcations debug console.
		hades::data::data_system _dataMan;					///< The applications resource loader
		StateManager _states;								///< The statemanager holds, ticks, and cleans up all of the game states.
		thread_pool _thread_pool;							///< App provided shared thread pool
		
		sf::View _overlay_view;								///< View for the debug overlays, matches screen resolution
		debug::overlay_manager _overlay_manager;			///< Manager for debug gui overlays
		std::optional<debug::text_overlay_manager> _text_overlay_manager;	///< Overlay manager for text based overlays.
		debug::screen_overlay_manager _screen_overlay_manager; ///< Manager for full screen overlays 
		std::optional<gui> _gui;							///< Gui for debug overlays to use
		bool _show_gui_demo = false;
		debug::console_overlay* _console_debug = nullptr;		///< The console interation devtool.

		sf::RenderWindow _window;							///< SFML window object. 

		struct framelimit_struct {
			bool vsync = false;
			int32 framelimit = 0;
		};
		framelimit_struct _framelimit;						///< Flag for SFML vsync
	};
};//hades

#endif //HADES_APP_HPP
