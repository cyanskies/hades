#ifndef HADES_CONSOLEVARS_HPP
#define HADES_CONSOLEVARS_HPP

#include "hades/console_variables.hpp"

#include <string_view>

namespace hades
{
	void create_core_console_variables();

	namespace cvars
	{
		//console variable namespaces
		//r_* render variable names
		// none currently?
		//s_* server variables
		// server framerate and so on
		//a_* for audio settings
		//n_* for network settings
		//editor_* for game editor settings
		//shared for level, mission and mod editors

		//client vars
		constexpr auto client_tick_time = "c_ticktime"; //the amount of time each tick represents
		constexpr auto client_max_tick = "c_maxframetime"; //the max amount of time that can be spent on a single frame

		//file vars
		constexpr auto file_portable = "file_portable"; //if portable is true, saves and configs are stored in game directory
														//rather than users home directory
		constexpr auto file_deflate = "file_deflate"; //if true, saves and configs are compressed before being written to disk

		//console vars
		constexpr auto console_charsize = "con_charsize"; //the character size to draw in the ingame console
		constexpr auto console_fade = "con_fade"; //the transparency of the backdrop for the console overlay

		//vid vars
		constexpr auto video_fullscreen = "vid_fullscreen"; // draw in fullscreen mode
		constexpr auto video_resizable = "vid_resizable"; // allow window to be freely resized by the user
		constexpr auto video_width = "vid_width"; // resolution width
		constexpr auto video_height = "vid_height"; // resolution height
		constexpr auto video_depth = "vid_depth"; // colour bit depth

		//server host and connection settings

		namespace default_value
		{
			constexpr auto video_fullscreen = false;
			constexpr auto video_resizable = false;
			constexpr auto video_width = 800;
			constexpr auto video_height = 600;
			constexpr auto video_depth = 32;
		}
	}
}

#endif //!HADES_CONSOLEVARS_HPP
