#ifndef HADES_BEGIN_HPP
#define HADES_BEGIN_HPP

#include <memory>

#include "Hades/data_system.hpp"
#include "hades/input.hpp"
#include "Hades/StateManager.hpp"
#include "hades/system.hpp"
#include "hades/types.hpp"

//handles application startup and command line parameters
int hades_main(int argc, char* argv[]);

/////////////////////////////////////
/// Called after defaultBindings
/// 
/// Allows the app to register its default game archive.
/// eg. return "hades";, will load hades.zip
/////////////////////////////////////
std::string_view defaultGame();

/////////////////////////////////////
/// Called before HadesMain defaultBindings
/// 
/// Allows the app to register its unique resource types.
///	Apps must register both yaml parsers for the resource and loaders
///
/// Apps may register their Actions here
/////////////////////////////////////
void resourceTypes(hades::data::data_system &data);

/////////////////////////////////////
/// Called at the start of the app
/// to set the starting state
///
/// Called after initial preperations and config files are loaded
///	allows users to set custom console functions and set the starting state
///
///	Allows users to define the ui for the game, both list commands and provide default binds for them.
/////////////////////////////////////
void hadesMain(hades::StateManager &state, hades::input_system &bindings, hades::command_list &commandLine);

#endif //HADES_BEGIN_HPP
