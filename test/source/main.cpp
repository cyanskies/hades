#include "Hades/Main.hpp"

#include <memory>

#include "Hades/App.hpp"
#include "hades/background.hpp"
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
	return "dev";
}

void resourceTypes(hades::data::data_system &data)
{
	hades::register_level_editor_resources(data);
	hades::create_level_editor_console_vars();
}

void hadesMain(hades::StateManager &state, hades::input_system &bind, hades::command_list &commandLine)
{
	hades::register_mouse_input(bind);
	
	using test_state = hades::level_editor;

	std::unique_ptr<hades::state> consolestate = std::make_unique<test_state>();
	state.push(std::move(consolestate));
}
