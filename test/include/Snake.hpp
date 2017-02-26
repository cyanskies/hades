#ifndef TEST_SNAKE_HPP
#define TEST_SNAKE_HPP

#include "Hades/State.hpp"

#include "Hades/DataManager.hpp"

//A snake game

class Snake final : public hades::State
{
public:
	void init();
	bool handleEvent(sf::Event &windowEvent);
	void update(const sf::Window &window);
	void update(sf::Time deltaTime);
	void draw(sf::RenderTarget &target);
	void cleanup();
	void reinit();
	void pause();
	void resume();
};

namespace snake
{
	using texture = hades::resources::texture;

	texture snake_head, snake_body, food_fruit;
}

#endif //TEST_SNAKE_HPP