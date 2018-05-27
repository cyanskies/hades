#include "Hades/Main.hpp"

#include <memory>

#include "Hades/App.hpp"
#include "Hades/common-input.hpp"

#include "OrthoTerrain/editor.hpp"
#include "OrthoTerrain/terrain.hpp"

#include "input_names.hpp"
#include "ConsoleTestState.hpp"
#include "snake_loader.hpp"

int main(int argc, char **argv)
{
	return hades_main(argc, argv);
}

std::string_view defaultGame()
{
	return "ortho_terrain";
}

void resourceTypes(hades::data::data_system &data)
{
	ortho_terrain::EnableTerrain(&data);
}

void hadesMain(hades::StateManager &state, hades::input_system &bind, hades::CommandList &commandLine)
{
	hades::RegisterMouseInput(bind);

	std::unique_ptr<hades::State> editorstate = std::make_unique<ortho_terrain::terrain_editor>();
	state.push(std::move(editorstate));
	return;

	std::unique_ptr<hades::State> consolestate = std::make_unique<ConsoleTestState>();
	state.push(std::move(consolestate));
}
