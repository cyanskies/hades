#ifndef HADES_APP_HPP
#define HADES_APP_HPP

#include <string>
#include <vector>

#include "SFML/Graphics/RenderWindow.hpp"

#include "Bind.hpp"
#include "ConsoleView.hpp"
#include "DataManager.hpp"
#include "Main.hpp"
#include "StateManager.hpp"

namespace hades
{
	class Console;
	class ResourceManager;

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
		////////////////////////////////////////////////////////////
		void handleEvents(State *activeState);

		////////////////////////////////////////////////////////////
		/// \brief Registers the engine provided console commands.
		///
		////////////////////////////////////////////////////////////
		void registerConsoleCommands();

		////////////////////////////////////////////////////////////
		/// Member Data
		////////////////////////////////////////////////////////////

		Bind _bindings;								///< Used by the console to provide bindable input.
		std::shared_ptr<Console> _console;			///< The appcations debug console.
		hades::DataManager _dataMan;				///< The applications resource loader
		std::shared_ptr<ResourceManager> _resource; ///< The resource manager for the entire application.(depreciated)
		StateManager _states;						///< The statemanager holds, ticks, and cleans up all of the game states.
		
		sf::RenderWindow _window;					///< SFML window object. 

		ConsoleView _consoleView;					///< The console interation devtool.
	};

};//hades

#endif //HADES_APP_HPP