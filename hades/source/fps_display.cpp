#include "hades/fps_display.hpp"

#include "SFML/Graphics/RenderTarget.hpp"

#include "hades/console_variables.hpp"
#include "hades/font.hpp"
#include "hades/utility.hpp"

namespace hades
{
	fps_overlay::fps_overlay(fps_mode mode)
		: _frame_time{console::get_float(cvars::client_previous_frametime)},
		_tick_per_frame{console::get_int(cvars::client_tick_count)},
		_tick_avg{console::get_float(cvars::client_avg_tick_time)},
		_tick_max{console::get_float(cvars::client_max_tick_time)},
		_tick_min{console::get_float(cvars::client_min_tick_time)},
		_tick_total{console::get_float(cvars::client_total_tick_time)},
		_draw_time{console::get_float(cvars::render_drawtime)},
		_font{data::get<resources::font>(resources::default_font_id())},
		_mode{mode}
	{
		const auto text_size = console::get_int(cvars::console_charsize, cvars::default_value::console_charsize);
		_text = sf::Text{ "", _font->value , integer_cast<unsigned int>(text_size->load()) };

		assert(mode >= fps_mode::off);
		assert(mode < fps_mode::last);
	}

	sf::Vector2f fps_overlay::getSize() const
	{
		const auto bounds = _text.getLocalBounds();
		return { bounds.width, bounds.height };
	}

	void fps_overlay::draw(time_duration, sf::RenderTarget &t, sf::RenderStates s)
	{
		auto str = string{};

		_text.setFillColor(sf::Color::White);

		if (_mode == frame_time)
		{
			str = "frametime: " + to_string(_frame_time->load()) + "ms";
		}
		else if (_mode == fps)
		{
			const auto ft = _frame_time->load();
			const auto time = ft == 0 ? 0.01f : ft;
			const auto fps = static_cast<int32>(1000 / time);
			str = "fps: " + to_string(fps);
			if (fps < 30)
				_text.setFillColor(sf::Color::Red);
			else if (fps < 60)
				_text.setFillColor(sf::Color::Yellow);
		}
		else if (_mode == diag)
		{
			str = "frametime: " + to_string(_frame_time->load()) + "ms";
			str += "\nticks per frame: " + to_string(_tick_per_frame->load());
			str += "\nmin ticktime: " + to_string(_tick_min->load()) + "ms\n"
				+ "max ticktime: " + to_string(_tick_max->load()) + "ms\n"
				+ "avg ticktime: " + to_string(_tick_avg->load()) + "ms\n"
				+ "update time: " + to_string(_tick_total->load()) + "ms\n"
				+ "draw time: " + to_string(_draw_time->load()) + "ms";
		}

		_text.setString(std::move(str));

		if (_mode != off)
			t.draw(_text, s);
	}

	static debug::Overlay* fps_display = nullptr;

	void create_fps_overlay(int32 mode)
	{
		using fps_mode = fps_overlay::fps_mode;

		if (mode <= fps_mode::off)
			fps_display = debug::DestroyOverlay(fps_display);
		else if (mode < fps_mode::last)
		{
			fps_display = debug::DestroyOverlay(fps_display);
			fps_display = debug::CreateOverlay(std::make_unique<fps_overlay>(static_cast<fps_mode>(mode)));
		}
	}
}
