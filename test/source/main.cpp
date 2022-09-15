#include "Hades/Main.hpp"

#include <memory>

#include "Hades/App.hpp"
#include "hades/background.hpp"
#include "hades/mission_editor.hpp"
#include "hades/mouse_input.hpp"
#include "Hades/Server.hpp"

#include "gui_state.hpp"
#include "simple_instance_state.hpp"

std::string_view defaultGame()
{
	return "dev";
}

void resourceTypes(hades::data::data_manager &data)
{
	hades::create_mission_editor_console_variables();
	hades::register_mission_editor_resources(data);
}

void hadesMain(hades::StateManager &state, hades::input_system &bind, hades::command_list &commandLine)
{
	hades::register_mouse_input(bind);
	
	using test_state = simple_instance_state;
	auto state_ptr = std::make_unique<gui_state>();
	state.push(std::move(state_ptr));
}

int main(int argc, char** argv)
{
	return hades::hades_main(argc, argv, defaultGame(), resourceTypes, hadesMain);
}
