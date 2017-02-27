#ifndef TEST_CONSOLETEST_HPP
#define TEST_CONSOLETEST_HPP

#include "SFML/Graphics/Sprite.hpp"
#include "SFML/Graphics/RectangleShape.hpp"

#include "Hades/DataManager.hpp"
#include "Hades/State.hpp"

class ConsoleTestState final : public hades::State
{
public:
	void init();
	bool handleEvent(sf::Event &windowEvent);
	void update(const sf::Window &window);
	void update(sf::Time deltaTime);
	void draw(sf::RenderTarget &target, sf::Time deltaTime);
	void cleanup();
	void reinit();
	void pause();
	void resume();
private:

	hades::resources::texture *ball, *missing;
	sf::Sprite ball_sprite, missing_sprite;
	sf::RectangleShape rect;

	float ball_direction;
};

#endif //TEST_CONSOLETEST_HPP