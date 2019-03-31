#include "Hades/Main.hpp"

#include <memory>

#include "Hades/App.hpp"
#include "hades/background.hpp"
#include "hades/mouse_input.hpp"
#include "Hades/Server.hpp"

#include "simple_instance_state.hpp"

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
	hades::register_game_server_resources(data);
}

void hadesMain(hades::StateManager &state, hades::input_system &bind, hades::command_list &commandLine)
{
	hades::register_mouse_input(bind);
	
	using test_state = simple_instance_state;
	auto state_ptr = std::make_unique<test_state>();
	state.push(std::move(state_ptr));
}
