#ifndef TEST_SIMPLE_INSTANCE_HPP
#define TEST_SIMPLE_INSTANCE_HPP

#include "Hades/State.hpp"

class simple_instance_state final : public hades::state
{
public:
	void init() override;
	bool handle_event(const hades::event &windowEvent) override;
	void update(hades::time_duration deltaTime, const sf::RenderTarget&, hades::input_system::action_set) override;
	void draw(sf::RenderTarget &target, hades::time_duration deltaTime) override;
	void reinit() override;
	void pause() override;
	void resume() override;

private:
	sf::View _view;
};

#endif //TEST_CONSOLETEST_HPP