#include "gui_state.hpp"

#include "SFML/Graphics/RectangleShape.hpp"

#include "hades/console_variables.hpp"
#include "hades/properties.hpp"
#include "hades/time.hpp"

void gui_state::init()
{
	reinit();
}

bool gui_state::handle_event(const hades::event &windowEvent)
{
	return _gui.handle_event(windowEvent);
}

void gui_state::update(hades::time_duration deltaTime, const sf::RenderTarget&, hades::input_system::action_set)
{
	_gui.update(deltaTime);
	_gui.frame_begin();
	_gui.show_demo_window();
	_gui.frame_end();
}

void gui_state::draw(sf::RenderTarget &target, hades::time_duration deltaTime)
{
	target.setView(_view);
	target.resetGLStates();
	target.draw(_gui);
}

void gui_state::reinit()
{
	const auto x = hades::console::get_int(hades::cvars::video_width);
	const auto y = hades::console::get_int(hades::cvars::video_height);

	_gui.set_display_size({ static_cast<float>(*x), static_cast<float>(*y) });

	_view.reset({ {}, {static_cast<float>(*x), static_cast<float>(*y)} });
}

void gui_state::pause()
{}

void gui_state::resume()
{}