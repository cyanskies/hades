#include "hades/main.hpp"
#include "hades/state.hpp"

class basic_state : public hades::state
{};

void app_main(hades::StateManager& state, hades::input_system& bindings, hades::command_list& commandLine)
{
	state.push(std::make_unique<basic_state>());
}

int main(int argc, char** argv)
{
	return hades::hades_main(argc, argv, "basic_app", nullptr, app_main);
}
