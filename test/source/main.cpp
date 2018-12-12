#include "Hades/Main.hpp"

#include <memory>

#include "Hades/App.hpp"

#include "hades/core_curves.hpp"
#include "hades/level_editor.hpp"
#include "hades/level_editor_grid.hpp"
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
	return "objects";
}

void resourceTypes(hades::data::data_system &data)
{
	hades::create_editor_console_variables();
	hades::create_level_editor_grid_variables();
	hades::register_level_editor_object_resources(data);
	hades::register_core_curves(data);
}

void hadesMain(hades::StateManager &state, hades::input_system &bind, hades::command_list &commandLine)
{
	hades::register_mouse_input(bind);
	
	//std::unique_ptr<hades::State> editorstate = std::make_unique<ortho_terrain::terrain_editor>();
	//state.push(std::move(editorstate));
	//return;

	using test_state = hades::basic_level_editor<hades::level_editor_level_props,
		hades::level_editor_objects, hades::level_editor_grid>;
	//using test_state = gui_state;

	std::unique_ptr<hades::state> consolestate = std::make_unique<test_state>();
	state.push(std::move(consolestate));
}
