#include "Hades/Main.hpp"

#include <memory>

#include "Hades/App.hpp"
#include "Hades/common-input.hpp"

#include "Objects/editor.hpp"
#include "Objects/resources.hpp"

#include "input_names.hpp"
#include "ConsoleTestState.hpp"
#include "snake_loader.hpp"

int main(int argc, char **argv)
{
	return hades_main(argc, argv);
}

std::string defaultGame()
{
	return "objects";
}

void resourceTypes(hades::data::data_manager &data)
{
	objects::RegisterObjectResources(&data);
	//data.register_resource_type("snake-rules", parseSnakeRules);

	//get names for the input system
	//auto move_left = data.getUid("move_left");
}

void hadesMain(hades::StateManager &state, hades::InputSystem &bind, hades::CommandList &commandLine)
{
	hades::RegisterMouseInput(bind);

	std::unique_ptr<hades::State> editorstate = std::make_unique<objects::object_editor>();
	state.push(std::move(editorstate));
	return;

	std::unique_ptr<hades::State> consolestate = std::make_unique<ConsoleTestState>();
	state.push(std::move(consolestate));
}
