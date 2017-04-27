#include "Hades/Main.hpp"

#include <memory>

#include "ConsoleTestState.hpp"
#include "snake_loader.hpp"

void defaultBindings(hades::InputSystem &bind)
{

}

std::string defaultGame()
{
	return "test";
}

void resourceTypes(hades::DataManager &data)
{
	data.register_resource_type("snake-rules", parseSnakeRules);
}

void hadesMain(hades::StateManager &state, std::shared_ptr<hades::Console> console, hades::CommandList &commandLine)
{
	std::unique_ptr<hades::State> consolestate = std::make_unique<ConsoleTestState>();
	state.push(std::move(consolestate));
}

