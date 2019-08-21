#include "hades/console_variables.hpp"

#include "hades/properties.hpp"
#include "hades/types.hpp"

namespace hades
{
	void create_core_console_variables()
	{
		#ifdef NDEBUG
				constexpr auto render_threads = cvars::default_value::render_threadcount;
		#else
				constexpr auto render_threads = int32{ 0 };
		#endif
		console::create_property(cvars::render_threadcount, render_threads);

		console::create_property(cvars::render_drawtime, cvars::default_value::render_drawtime);

		#ifdef NDEBUG
				constexpr auto server_threads = cvars::default_value::server_threadcount;
		#else
				constexpr auto server_threads = int32{ 0 };
		#endif
		console::create_property(cvars::server_threadcount, server_threads);

		console::create_property(cvars::client_tick_rate, cvars::default_value::client_tickrate);
		console::create_property(cvars::client_avg_tick_time, cvars::default_value::client_avg_tick_time);
		console::create_property(cvars::client_max_tick_time, cvars::default_value::client_max_tick_time);
		console::create_property(cvars::client_min_tick_time, cvars::default_value::client_min_tick_time);
		console::create_property(cvars::client_total_tick_time, cvars::default_value::client_total_tick_time);
		console::create_property(cvars::client_max_tick, cvars::default_value::client_tick_max);
		console::create_property(cvars::client_previous_frametime, cvars::default_value::client_previous_frametime);
		console::create_property(cvars::client_tick_count, cvars::default_value::client_tick_count);

		//in debug we want portable = true 
		//					deflate = false
		// and the reverse in release
		#ifdef NDEBUG
			constexpr auto file_defaults = false;
		#else
			constexpr auto file_defaults = true;
		#endif

		console::create_property(cvars::file_portable,	file_defaults);
		console::create_property(cvars::file_deflate,	!file_defaults);

		console::create_property(cvars::console_charsize, cvars::default_value::console_charsize);
		console::create_property(cvars::console_fade, cvars::default_value::console_fade);

		console::create_property(cvars::video_fullscreen,	cvars::default_value::video_fullscreen);
		console::create_property(cvars::video_resizable,	cvars::default_value::video_resizable);
		console::create_property(cvars::video_width,		cvars::default_value::video_width);
		console::create_property(cvars::video_height,		cvars::default_value::video_height);
		console::create_property(cvars::video_depth,		cvars::default_value::video_depth);
	}
}
