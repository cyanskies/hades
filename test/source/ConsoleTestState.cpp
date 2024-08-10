#include "ConsoleTestState.hpp"

#include "Hades/Data.hpp"
#include "Hades/Logging.hpp"
#include "Hades/Properties.hpp"
#include "hades/texture.hpp"
#include "Hades/Types.hpp"

using namespace hades::types;

void ConsoleTestState::init()
{
	LOG("Console test");

	hades::console::set_property("int8", 5);
	auto int8val = hades::console::get_int("int8", -1);
	hades::console::set_property("uint8", 5);
	auto uintval = hades::console::get_int("uint8", -1);
	hades::console::set_property("uint8_under", -5);

	assert(*uintval == 5);
	//try to overwrite with wrong type
	hades::console::set_property("int8", "error");
	assert(*int8val == 5);

	//assign the needed textures
	auto missing_id = hades::data::get_uid("example"), ball_id = hades::data::get_uid("ball");

	namespace tex = hades::resources::texture_functions;
	missing = tex::get_resource(missing_id);
	ball = tex::get_resource(ball_id);

	assert(missing && ball);

	missing_sprite.setTexture(tex::get_sf_texture(missing));
	ball_sprite.setTexture(tex::get_sf_texture(ball));

	missing_sprite.setPosition({ 20.f, 30.f });

	ball_sprite.setPosition({330.f, 230.f});
	ball_direction = -5.f;

	rect.setPosition({ 20.f, 500.f });
	rect.setFillColor(sf::Color::Blue);
	rect.setSize({ 300.f, 90.f });
}

void ConsoleTestState::update(hades::time_duration, const sf::RenderTarget&, const hades::input_system::action_set&)
{
	if (ball_sprite.getPosition().x + ball_sprite.getLocalBounds().width > 800.f)
		ball_direction = -5.f;
	else if (ball_sprite.getPosition().x < 0)
		ball_direction = 5.f;

	ball_sprite.move({ ball_direction, 0.f });
}

void ConsoleTestState::draw(sf::RenderTarget &target, hades::time_duration)
{
	target.draw(missing_sprite);
	target.draw(ball_sprite);

	target.draw(rect);
}