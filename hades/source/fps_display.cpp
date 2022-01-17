#include "hades/fps_display.hpp"

#include "SFML/Graphics/RenderTarget.hpp"

#include "hades/console_variables.hpp"
#include "hades/font.hpp"
#include "hades/utility.hpp"

using namespace std::string_literals;

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
		_mode{mode}
	{
		assert(mode >= fps_mode::off);
		assert(mode < fps_mode::last);
	}

	string fps_overlay::update()
	{
		auto str = string{};

		if (_mode == frame_time)
		{
			str = "frametime: "s + to_string(_frame_time->load()) + "ms"s;
		}
		else if (_mode == fps)
		{
			const auto ft = _frame_time->load();
			const auto time = ft == 0 ? 0.01f : ft;
			const auto fps = static_cast<int32>(1000 / time);
			str = "fps: " + to_string(fps);
		}
		else if (_mode == diag)
		{
			str = "frametime: "s + to_string(_frame_time->load()) + "ms\n"s
				+ "ticks per frame: "s + to_string(_tick_per_frame->load())
				+ "\nmin ticktime: "s + to_string(_tick_min->load()) + "ms\n"s
				+ "max ticktime: "s + to_string(_tick_max->load()) + "ms\n"s
				+ "avg ticktime: "s + to_string(_tick_avg->load()) + "ms\n"s
				+ "update time: "s + to_string(_tick_total->load()) + "ms\n"s
				+ "draw time: "s + to_string(_draw_time->load()) + "ms"s;
		}

		return str;
	}

	static debug::text_overlay* fps_display = nullptr;

	void create_fps_overlay(int32 mode)
	{
		using fps_mode = fps_overlay::fps_mode;

		if (mode <= fps_mode::off)
			fps_display = debug::destroy_text_overlay(fps_display);
		else if (mode < fps_mode::last)
		{
			fps_display = debug::destroy_text_overlay(fps_display);
			fps_display = debug::create_text_overlay(std::make_unique<fps_overlay>(static_cast<fps_mode>(mode)));
		}
	}
}
