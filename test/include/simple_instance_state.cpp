#include "simple_instance_state.hpp"

void simple_instance_state::init()
{
}

bool simple_instance_state::handle_event(const hades::event & windowEvent)
{
	return false;
}

void simple_instance_state::update(hades::time_duration deltaTime, const sf::RenderTarget &, hades::input_system::action_set)
{
}

void simple_instance_state::draw(sf::RenderTarget & target, hades::time_duration deltaTime)
{
}

void simple_instance_state::reinit()
{
}

void simple_instance_state::pause()
{
}

void simple_instance_state::resume()
{
}
