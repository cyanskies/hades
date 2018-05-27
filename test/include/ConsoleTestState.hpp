#ifndef TEST_CONSOLETEST_HPP
#define TEST_CONSOLETEST_HPP

#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/RectangleShape.hpp"

#include "Hades/simple_resources.hpp"
#include "Hades/State.hpp"

class ConsoleTestState final : public hades::State
{
public:
	void init() override;
	bool handleEvent(const hades::event &windowEvent) override;
	void update(sf::Time deltaTime, const sf::RenderTarget&, hades::input_system::action_set) override;
	void draw(sf::RenderTarget &target, sf::Time deltaTime) override;
	void reinit() override;
	void pause() override;
	void resume() override;
private:

	const hades::resources::texture *ball, *missing;
	sf::Sprite ball_sprite, missing_sprite;
	sf::RectangleShape rect;

	float ball_direction;
};

#endif //TEST_CONSOLETEST_HPP