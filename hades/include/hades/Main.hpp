#ifndef HADES_BEGIN_HPP
#define HADES_BEGIN_HPP

#include "hades/input.hpp"
#include "hades/StateManager.hpp"
#include "hades/system.hpp"

namespace hades::data
{
	class data_manager;
}

namespace hades
{
	/////////////////////////////////////
	/// Called before app_main
	/// 
	/// Allows the app to register its unique resource types.
	///	Apps must register both yaml parsers for the resource and loaders
	///
	/// Apps may register their Actions here
	/////////////////////////////////////
	using register_resource_types_fn = void(*)(hades::data::data_manager&);
	
	/////////////////////////////////////
	/// Called at the start of the app
	/// to set the starting state
	///
	///	allows users to set custom console functions and set the starting state
	///
	///	Allows users to define the ui for the game, both list commands and provide default binds for them.
	/////////////////////////////////////
	using app_main_fn = void(*)(hades::StateManager&, hades::input_system&, hades::command_list&);

	// call to let hades-app manage the main function
	int hades_main(int argc, char* argv[], std::string_view game, 
		register_resource_types_fn = nullptr,
		app_main_fn = nullptr);
}

#endif //HADES_BEGIN_HPP
