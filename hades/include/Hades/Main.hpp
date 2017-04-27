#ifndef HADES_BEGIN_HPP
#define HADES_BEGIN_HPP

#include <memory>

#include "Hades/Console.hpp"
#include "Hades/DataManager.hpp"
#include "Hades/Input.hpp"
#include "Hades/StateManager.hpp"
#include "Hades/Types.hpp"

namespace hades
{
	typedef std::vector<types::string> CommandList;
}

/////////////////////////////////////
///	allows users to define the ui for the game, both list commands and provide default binds for them.
/////////////////////////////////////
void defaultBindings(hades::InputSystem &bindings);

/////////////////////////////////////
/// Called after defaultBindings
/// 
/// Allows the app to register its default game archive.
/// eg. return "hades";, will load hades.zip
/////////////////////////////////////
hades::types::string defaultGame();

/////////////////////////////////////
/// Called after defaultBindings
/// 
/// Allows the app to register its unique resource types.
///	Apps must register both yaml parsers for the resource and loaders
///
/// Apps may register their Actions here
/////////////////////////////////////
void resourceTypes(hades::DataManager &data);

/////////////////////////////////////
/// Called at the start of the app
/// to set the starting state
///
/// Called after initial preperations and config files are loaded
///	allows users to set custom console functions and set the starting state
/////////////////////////////////////
void hadesMain(hades::StateManager &state, std::shared_ptr<hades::Console> console, hades::CommandList &commandLine);

#endif //HADES_BEGIN_HPP
