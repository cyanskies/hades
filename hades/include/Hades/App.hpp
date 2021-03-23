#ifndef HADES_APP_HPP
#define HADES_APP_HPP

#include <string>
#include <vector>

#include "SFML/Graphics/RenderWindow.hpp"

#include "hades/async.hpp"
#include "Hades/Console.hpp"
#include "Hades/data_system.hpp"
#include "Hades/Debug.hpp"
#include "hades/sf_input.hpp"
#include "Hades/Main.hpp"
#include "Hades/StateManager.hpp"
#include "hades/system.hpp"

namespace hades
{
	class ConsoleView;

	////////////////////////////////////////////////////////////
	/// \brief Main application class.
	///
	////////////////////////////////////////////////////////////
	class App
	{
	public: 
		////////////////////////////////////////////////////////////
		/// \brief Creates and starts most of the apps subsystems and config files.
		///
		////////////////////////////////////////////////////////////
		void init();

		////////////////////////////////////////////////////////////
		/// \brief Runs the last few init steps after letting command line variables run.
		///
		///	\param commands The command line arguments.
		///
		////////////////////////////////////////////////////////////
		void postInit(command_list commands);

		////////////////////////////////////////////////////////////
		/// \brief Starts the main app loop.
		///
		/// \return True if the app closed successfully, false otherwise.
		///
		////////////////////////////////////////////////////////////
		bool run();

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
		std::vector<input_event_system::checked_event> handleEvents(state *activeState);

		////////////////////////////////////////////////////////////
		/// \brief Registers the engine provided console commands.
		///
		////////////////////////////////////////////////////////////
		void registerConsoleCommands();

		////////////////////////////////////////////////////////////
		/// Member Data
		////////////////////////////////////////////////////////////

		input_event_system _input;					///< Used by the console to provide bindable input.
		Console _console;							///< The appcations debug console.
		hades::data::data_system _dataMan;			///< The applications resource loader
		StateManager _states;						///< The statemanager holds, ticks, and cleans up all of the game states.
		thread_pool _thread_pool;					///< App provided shared thread pool
		
		debug::OverlayManager _overlayMan;			///< The debug overlay manager.
		sf::RenderWindow _window;					///< SFML window object. 
		bool _sfVSync = false;						///< Flag for SFML sync

		ConsoleView *_consoleView = nullptr;		///< The console interation devtool.
	};

	bool LoadCommand(command_list&, std::string_view, console::function_no_argument);
	bool LoadCommand(command_list&, std::string_view, console::function);
};//hades

#endif //HADES_APP_HPP
