#include "hades/mod_editor.hpp"

#include "hades/console_variables.hpp"
#include "hades/properties.hpp"

using namespace std::string_view_literals;

namespace hades
{
	void mod_editor_impl::init()
	{
		reinit();
	}
	
	bool mod_editor_impl::handle_event(const event& e)
	{
		_gui.activate_context();
		return _gui.handle_event(e);
	}

	void mod_editor_impl::update(time_duration dt, const sf::RenderTarget& t, input_system::action_set)
	{
		_gui.activate_context();
		_gui.update(dt);
		_gui.frame_begin();

		auto [data_man, lock] = data::detail::get_data_manager_exclusive_lock();
		std::ignore = lock;
		_inspector.update(_gui, data_man);
		_gui.frame_end();
	}

	void mod_editor_impl::draw(sf::RenderTarget& r, time_duration)
	{
		r.setView(_gui_view);
		r.draw(_backdrop);
		_gui.activate_context();
		r.draw(_gui);
	}
	
	void mod_editor_impl::reinit()
	{
		const auto width = console::get_int(cvars::video_width, cvars::default_value::video_width);
		const auto height = console::get_int(cvars::video_height, cvars::default_value::video_height);

		const auto fwidth = static_cast<float>(width->load());
		const auto fheight = static_cast<float>(height->load());

		_gui_view.reset({ {}, { fwidth, fheight } });
		_gui.activate_context();
		_gui.set_display_size({ fwidth, fheight });

		_backdrop.setPosition({ 0.f, 0.f });
		_backdrop.setSize({ fwidth, fheight });
		const auto background_colour = sf::Color{ 200u, 200u, 200u, 255u };
		_backdrop.setFillColor(background_colour);
	}
}
