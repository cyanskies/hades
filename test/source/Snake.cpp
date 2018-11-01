#include "Snake.hpp"

#include "Hades/Logging.hpp"
#include "Hades/Types.hpp"

#include "snake_loader.hpp"

using namespace hades::types;

void Snake::init()
{
	LOG("Starting Snake");

	//make game board

	//reset gameinstance

	//send starting data to the gamerenderer
}

bool Snake::handle_event(const hades::event &windowEvent)
{
	return false;
}

void Snake::update(hades::time_duration deltaTime, const sf::RenderTarget&, hades::input_system::action_set actions)
{}

void Snake::draw(sf::RenderTarget &target, hades::time_duration deltaTime)
{}

void Snake::reinit()
{}

void Snake::pause()
{}

void Snake::resume()
{}