#include "hades/console_variables.hpp"

#include "hades/properties.hpp"
#include "Hades/types.hpp"

namespace hades
{
	void create_core_console_variables()
	{
		console::create_property<int32>(cvars::client_tick_time,	30);
		console::create_property<int32>(cvars::client_max_tick,		150);

		//in debug we want portable = true 
		//					deflate = false
		// and the reverse in release
		#ifdef NDEBUG
			constexpr auto file_defaults = false;
		#else
			constexpr auto file_defaults = true;
#		endif

		console::create_property(cvars::file_portable,	file_defaults);
		console::create_property(cvars::file_deflate,	!file_defaults);

		console::create_property(cvars::video_fullscreen,	cvars::default_value::video_fullscreen);
		console::create_property(cvars::video_resizable,	cvars::default_value::video_resizable);
		console::create_property(cvars::video_width,		cvars::default_value::video_width);
		console::create_property(cvars::video_height,		cvars::default_value::video_height);
		console::create_property(cvars::video_depth,		cvars::default_value::video_depth);
	}
}