#ifndef TEST_CONSOLETEST_HPP
#define TEST_CONSOLETEST_HPP

#include "Hades/State.hpp"

class ConsoleTestState final : public hades::State
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

#endif //TEST_CONSOLETEST_HPP