#ifndef HADES_FPS_DISPLAY_HPP
#define HADES_FPS_DISPLAY_HPP

#include "SFML/Graphics/Text.hpp"

#include "Hades/Debug.hpp"
#include "hades/font.hpp"
#include "hades/properties.hpp"

namespace hades
{
	class fps_overlay final : public debug::text_overlay
	{
	public:
		enum fps_mode //TODO: enum class
		{
			off,
			diag,
			frame_time,
			fps,
			last
		};

		fps_overlay(fps_mode mode);
		string update() override;

	private:
		console::property_float _frame_time;
		console::property_int _tick_per_frame;
		console::property_float _tick_avg;
		console::property_float _tick_max;
		console::property_float _tick_min;
		console::property_float _tick_total;
		console::property_float _draw_time;

		fps_mode _mode{ off };
	};

	void create_fps_overlay(int32 param);
}

#endif //!HADES_FPS_DISPLAY_HPP