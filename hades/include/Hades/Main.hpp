#ifndef HADES_BEGIN_HPP
#define HADES_BEGIN_HPP

#include <memory>

#include "Hades/data_system.hpp"
#include "Hades/Input.hpp"
#include "Hades/StateManager.hpp"
#include "Hades/Types.hpp"

namespace hades
{
	using ArgumentList = std::vector<std::string_view>;

	struct Command
	{
		Command() {}
		Command(std::string_view sv);
		Command(std::string_view command, ArgumentList args) : command(command),
			arguments(args)
		{}

		std::string_view command;
		ArgumentList arguments;
	};

	bool operator<(const Command& lhs, const Command &rhs);
	bool operator==(const Command& lhs, const Command &rhs);

	types::string to_string(const Command&);

	typedef std::vector<Command> CommandList;
}

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
void hadesMain(hades::StateManager &state, hades::InputSystem &bindings, hades::CommandList &commandLine);

#endif //HADES_BEGIN_HPP
