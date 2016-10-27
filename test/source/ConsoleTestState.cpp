#include "ConsoleTestState.hpp"

#include "Hades/Types.hpp"

using namespace hades::types;

void ConsoleTestState::init()
{
	console->echo("Console test");

	console->set("int8", int8(5));
	auto int8val = console->getValue<int8>("int8");
	console->set("uint8", uint8(5));
	auto uintval = console->getValue<uint8>("uint8");
	console->set("uint8_under", uint8(-5));

	assert(*uintval == 5);
	//try to overwrite with wrong type
	console->set("int8", int16(15));
	assert(*int8val == 5);
}

bool ConsoleTestState::handleEvent(sf::Event &windowEvent)
{
	return false;
}

void ConsoleTestState::update(const sf::Window &window)
{}

void ConsoleTestState::update(sf::Time deltaTime)
{}

void ConsoleTestState::draw(sf::RenderTarget &target)
{}

void ConsoleTestState::cleanup()
{}

void ConsoleTestState::reinit()
{}

void ConsoleTestState::pause()
{}

void ConsoleTestState::resume()
{}