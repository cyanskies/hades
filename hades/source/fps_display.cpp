#include "hades/fps_display.hpp"

#include "SFML/Graphics/RenderTarget.hpp"

#include "hades/console_variables.hpp"
#include "hades/font.hpp"
#include "hades/utility.hpp"

namespace hades
{
	fps_overlay::fps_overlay(fps_mode mode)
		: _frame_time{console::get_float(cvars::client_previous_frametime)},
		_font{data::get<resources::font>(resources::default_font_id())},
		_mode{mode}
	{
		const auto text_size = console::get_int(cvars::console_charsize, cvars::default_value::console_charsize);
		_text = sf::Text{ "", _font->value , integer_cast<unsigned int>(text_size->load()) };

		assert(mode >= fps_mode::off);
		assert(mode <= fps_mode::fps);
	}

	sf::Vector2f fps_overlay::getSize() const
	{
		const auto bounds = _text.getLocalBounds();
		return { bounds.width, bounds.height };
	}

	void fps_overlay::draw(time_duration dt, sf::RenderTarget &t, sf::RenderStates s)
	{
		if (_mode == frame_time)
			_text.setString("frametime: " + to_string(_frame_time->load()) + "ms");
		else if (_mode == fps)
		{
			const auto ft = _frame_time->load();
			const auto time = ft == 0 ? 0.01f : ft;
			_text.setString("fps: " + to_string(1000 / time) + "ms");
		}
		else
			_text.setString("");

		t.draw(_text, s);
	}

	static debug::Overlay* fps_display = nullptr;

	void create_fps_overlay(int32 mode)
	{
		using fps_mode = fps_overlay::fps_mode;

		if (mode <= fps_mode::off)
			fps_display = debug::DestroyOverlay(fps_display);
		else if (mode <= fps_mode::fps)
		{
			fps_display = debug::DestroyOverlay(fps_display);
			fps_display = debug::CreateOverlay(std::make_unique<fps_overlay>(static_cast<fps_mode>(mode)));
		}
	}
}
