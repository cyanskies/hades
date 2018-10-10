#ifndef TEST_SNAKE_HPP
#define TEST_SNAKE_HPP

#include "Hades/State.hpp"

#include "hades/texture.hpp"
#include "Hades/GameInstance.hpp"

//A snake game

class Snake final : public hades::State
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
	hades::GameInstance _gameLogic;
};

namespace snake
{
	using texture = hades::resources::texture;

	//texture snake_head, snake_body, food_fruit;
}

#endif //TEST_SNAKE_HPP