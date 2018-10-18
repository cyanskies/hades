#include "Hades/Main.hpp"

#include <memory>

#include "Hades/App.hpp"

#include "gui_state.hpp"

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
}

void hadesMain(hades::StateManager &state, hades::input_system &bind, hades::command_list &commandLine)
{
	//hades::RegisterMouseInput(bind);

	//std::unique_ptr<hades::State> editorstate = std::make_unique<ortho_terrain::terrain_editor>();
	//state.push(std::move(editorstate));
	//return;

	std::unique_ptr<hades::State> consolestate = std::make_unique<gui_state>();
	state.push(std::move(consolestate));
}
