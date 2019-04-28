#ifndef TEST_SIMPLE_INSTANCE_HPP
#define TEST_SIMPLE_INSTANCE_HPP

#include <memory>

#include "hades/data.hpp"
#include "hades/render_instance.hpp"
#include "Hades/render_interface.hpp"
#include "hades/Server.hpp"
#include "hades/state.hpp"
#include "Hades/timers.hpp"

void register_simple_instance_resources(hades::data::data_manager&);

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
	hades::server_ptr _server;
	hades::level_ptr _level = nullptr;
	hades::render_instance _client_instance;
	hades::render_interface _drawable_output;
	hades::time_point _current_time;
};

#endif //TEST_CONSOLETEST_HPP