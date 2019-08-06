#ifndef HADES_CONSOLEVARS_HPP
#define HADES_CONSOLEVARS_HPP

#include <string_view>

#include "hades/types.hpp"

namespace hades
{
	//called automatically by app.cpp
	void create_core_console_variables();

	namespace cvars
	{
		//console variable namespaces
		//r_* render variable names
		// none currently?
		//s_* server variables
		// server framerate and so on
		//c_* client variables
		// main game loop comtrols
		//a_* for audio settings
		//n_* for network settings
		//editor_* for game editor settings
		//shared for level, mission and mod editors

		//server vars
		constexpr auto server_threadcount = "s_threads"; //number of threads to use in the game and rendering server
														// -1 = auto, 0/1 = no threading, otherwise the number of threads to use
		//client vars
		constexpr auto client_tick_rate = "c_tickrate"; //number of ticks to calculate per second
		constexpr auto client_max_tick = "c_maxframetime"; //the max amount of time that can be spent on a single frame in ms
		constexpr auto client_previous_frametime = "c_previous_frametime"; // time taken to generate the last frame
		constexpr auto client_tick_count = "c_ticks_per_frame"; //number of ticks taken to generate the previous frame

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

		//debug vars
		//TODO: remove, this is handled by a console function
		constexpr auto fps = "fps"; // 0 = off; 1 = show frametime in ms; 2 = show fps

		//server host and connection settings

		namespace default_value
		{
			constexpr auto server_threadcount = -1;

			constexpr auto client_tickrate = 30;
			constexpr auto client_tick_time = 30;
			constexpr auto client_tick_max = 150;
			constexpr auto client_previous_frametime = -1.f;
			constexpr auto client_tick_count = 0;

			constexpr auto file_portable = false;
			constexpr auto file_deflate = true;

			constexpr auto console_charsize = 15;
			constexpr auto console_fade = 180;

			constexpr auto video_fullscreen = false;
			constexpr auto video_resizable = false;
            constexpr int32 video_width = 800;
            constexpr int32 video_height = 600;
            constexpr int32 video_depth = 32;

			constexpr int32 fps = 0;
		}
	}
}

#endif //!HADES_CONSOLEVARS_HPP
