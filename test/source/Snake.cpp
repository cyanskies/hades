#include "Snake.hpp"

#include "Hades/Types.hpp"

using namespace hades::types;

void Snake::init()
{
	console->echo("Starting Snake");
}

bool Snake::handleEvent(sf::Event &windowEvent)
{
	return false;
}

void Snake::update(const sf::Window &window)
{}

void Snake::update(sf::Time deltaTime)
{}

void Snake::draw(sf::RenderTarget &target, sf::Time deltaTime)
{}

void Snake::cleanup()
{}

void Snake::reinit()
{}

void Snake::pause()
{}

void Snake::resume()
{}