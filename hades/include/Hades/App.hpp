#ifndef HADES_APP_HPP
#define HADES_APP_HPP

#include <string>
#include <vector>

#include "SFML/Graphics/RenderWindow.hpp"

#include "Hades/Console.hpp"
#include "Hades/data_manager.hpp"
#include "Hades/Debug.hpp"
#include "Hades/Input.hpp"
#include "Hades/Main.hpp"
#include "Hades/StateManager.hpp"

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

		App();

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
		void postInit(CommandList commands);

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
		std::vector<Event> handleEvents(State *activeState);

		////////////////////////////////////////////////////////////
		/// \brief Registers the engine provided console commands.
		///
		////////////////////////////////////////////////////////////
		void registerConsoleCommands();

		////////////////////////////////////////////////////////////
		/// Member Data
		////////////////////////////////////////////////////////////

		InputSystem _input;							///< Used by the console to provide bindable input.
		Console _console;							///< The appcations debug console.
		hades::data::data_manager _dataMan;				///< The applications resource loader
		StateManager _states;						///< The statemanager holds, ticks, and cleans up all of the game states.
		
		debug::OverlayManager _overlayMan;			///< The debug overlay manager.
		sf::RenderWindow _window;					///< SFML window object. 
		bool _sfVSync;								///< Flag for SFML sync

		ConsoleView *_consoleView;					///< The console interation devtool.
	};

	bool LoadCommand(CommandList&, std::string_view, console::function);
};//hades

#endif //HADES_APP_HPP
