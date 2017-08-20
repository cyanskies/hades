#include "ConsoleTestState.hpp"

#include "Hades/Logging.hpp"
#include "Hades/Properties.hpp"
#include "Hades/Types.hpp"

using namespace hades::types;

void ConsoleTestState::init()
{
	hades::data_manager->load();

	LOG("Console test");

	hades::console::SetProperty("int8", 5);
	auto int8val = hades::console::GetInt("int8", -1);
	hades::console::SetProperty("uint8", 5);
	auto uintval = hades::console::GetInt("uint8", -1);
	hades::console::SetProperty("uint8_under", -5);

	assert(*uintval == 5);
	//try to overwrite with wrong type
	hades::console::SetProperty("int8", "error");
	assert(*int8val == 5);

	//assign the needed textures
	auto missing_id = hades::data_manager->getUid("example"), ball_id = hades::data_manager->getUid("ball");

	missing = hades::data_manager->getTexture(missing_id);
	ball = hades::data_manager->getTexture(ball_id);

	assert(missing && ball);

	missing_sprite.setTexture(missing->value);
	ball_sprite.setTexture(ball->value);

	missing_sprite.setPosition(20.f, 30.f);

	ball_sprite.setPosition(330.f, 230.f);
	ball_direction = -5.f;

	rect.setPosition(20.f, 500.f);
	rect.setFillColor(sf::Color::Blue);
	rect.setSize({ 300.f, 90.f });
}

bool ConsoleTestState::handleEvent(const hades::Event &windowEvent)
{
	return false;
}

void ConsoleTestState::update(sf::Time deltaTime, const sf::RenderTarget&, hades::InputSystem::action_set actions)
{
	if (ball_sprite.getPosition().x + ball_sprite.getLocalBounds().width > 800.f)
		ball_direction = -5.f;
	else if (ball_sprite.getPosition().x < 0)
		ball_direction = 5.f;

	ball_sprite.move(ball_direction, 0.f);
}

void ConsoleTestState::draw(sf::RenderTarget &target, sf::Time deltaTime)
{
	target.draw(missing_sprite);
	target.draw(ball_sprite);

	target.draw(rect);
}

void ConsoleTestState::cleanup()
{}

void ConsoleTestState::reinit()
{}

void ConsoleTestState::pause()
{}

void ConsoleTestState::resume()
{}