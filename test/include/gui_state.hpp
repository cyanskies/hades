#ifndef TEST_GUI_HPP
#define TEST_GUI_HPP

#include "hades/gui.hpp"
#include "Hades/State.hpp"

class gui_state final : public hades::State
{
public:
	void init() override;
	bool handle_event(const hades::event &windowEvent) override;
	void update(sf::Time deltaTime, const sf::RenderTarget&, hades::input_system::action_set) override;
	void draw(sf::RenderTarget &target, sf::Time deltaTime) override;
	void reinit() override;
	void pause() override;
	void resume() override;
private:
	sf::View _view;
	hades::gui _gui;
};

#endif //TEST_CONSOLETEST_HPP