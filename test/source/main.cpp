#include "Hades/Main.hpp"

#include <memory>

#include "Hades/App.hpp"
#include "Hades/common-input.hpp"

#include "OrthoTerrain/editor.hpp"
#include "OrthoTerrain/resources.hpp"

#include "input_names.hpp"
#include "ConsoleTestState.hpp"
#include "snake_loader.hpp"

int main(int argc, char **argv)
{
	return hades_main(argc, argv);
}

std::string defaultGame()
{
	return "example";
}

void resourceTypes(hades::DataManager &data)
{
	ortho_terrain::RegisterOrthoTerrainResources(&data);
	data.register_resource_type("snake-rules", parseSnakeRules);

	//get names for the input system
	auto move_left = data.getUid("move_left");
}

void hadesMain(hades::StateManager &state, hades::InputSystem &bind, hades::CommandList &commandLine)
{
	hades::RegisterMouseInput(bind, hades::data_manager);

	state.push(std::make_unique<ortho_terrain::terrain_editor>());
	return;

	std::unique_ptr<hades::State> consolestate = std::make_unique<ConsoleTestState>();
	state.push(std::move(consolestate));
}
