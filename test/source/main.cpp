#include "Hades/Main.hpp"

#include "ConsoleTestState.hpp"

void defaultBindings(hades::Bind &bind)
{

}

void hadesMain(hades::StateManager &state, std::shared_ptr<hades::Console> console, hades::CommandList &commandLine)
{
	std::unique_ptr<hades::State> consolestate = std::make_unique<ConsoleTestState>();
	state.push(consolestate);
}

