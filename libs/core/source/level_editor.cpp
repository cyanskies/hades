#include "hades/level_editor.hpp"

namespace hades::detail
{
	bool level_editor_impl::handle_event(const event &e)
	{
		return _gui.handle_event(e);
	}

	void level_editor_impl::reinit()
	{
		const auto width = console::get_int(cvars::video_width);
		const auto height = console::get_int(cvars::video_height);

		_window_width = static_cast<float>(*width);
		_window_height = static_cast<float>(*height);

		_gui.set_display_size({ _window_width, _window_height });
	}
}

void hades::create_editor_console_varaibles()
{
}
