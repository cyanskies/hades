#include "Hades/Main.hpp"

#include <memory>

#include "Hades/App.hpp"

#include "hades/level_editor.hpp"
#include "hades/level_editor_level_properties.hpp"
#include "hades/level_editor_objects.hpp"
#include "hades/mouse_input.hpp"

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
	hades::create_editor_console_variables();
}

void hadesMain(hades::StateManager &state, hades::input_system &bind, hades::command_list &commandLine)
{
	hades::register_mouse_input(bind);
	
	//std::unique_ptr<hades::State> editorstate = std::make_unique<ortho_terrain::terrain_editor>();
	//state.push(std::move(editorstate));
	//return;

	using test_state = hades::basic_level_editor<hades::level_editor_level_props, hades::level_editor_objects>;
	//using test_state = gui_state;

	std::unique_ptr<hades::state> consolestate = std::make_unique<test_state>();
	state.push(std::move(consolestate));
}
