#include "Hades/Main.hpp"

#include <memory>

#include "input_names.hpp"
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

	//get names for the input system
	auto move_left = data.getUid("move_left");
}

void hadesMain(hades::StateManager &state, hades::Console &console, hades::CommandList &commandLine)
{
	std::unique_ptr<hades::State> consolestate = std::make_unique<ConsoleTestState>();
	state.push(std::move(consolestate));
}
