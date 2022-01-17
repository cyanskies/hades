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
		//m_ for mission
		//l_ for level

		//render vars
		constexpr auto render_threadcount = "r_threads"; // [[deprecated]] same as s_threadcount
		constexpr auto render_drawtime = "r_drawtime"; //reports the time taken to generate and display the last frame in ms

		//server vars
		constexpr auto server_threadcount = "s_threads"; //number of threads to use in the game server
														// -1 = auto, 0/1 = no threading, otherwise the number of threads to use
		//client vars
		constexpr auto client_tick_rate = "c_tickrate"; //number of ticks to calculate per second
		constexpr auto client_avg_tick_time = "c_avg_tick_time"; // reports the average tick time of the frame in ms
		constexpr auto client_max_tick_time = "c_max_tick_time"; // reports the longest tick time of the frame in ms
		constexpr auto client_min_tick_time = "c_min_tick_time"; // reports the longest tick time of the frame in ms
		constexpr auto client_total_tick_time = "c_total_tick_time"; //reports the total time taken to do liguc update 
		constexpr auto client_max_tick = "c_max_allowed_frametime"; // [[deprecated]] the max amount of time that can be spent on a single frame in ms
		constexpr auto client_previous_frametime = "c_previous_frametime"; // reports time taken to generate the last frame
		constexpr auto client_tick_count = "c_ticks_per_frame"; // reports number of ticks taken to generate the previous frame

		//file vars
		constexpr auto file_portable = "file_portable"; //if portable is true, saves and configs are stored in game directory
														//rather than users home directory
		constexpr auto file_deflate = "file_deflate"; //if true, saves and configs are compressed before being written to disk

		//console vars
		//TODO: both deprecated
		constexpr auto console_charsize = "con_charsize"; //the character size to draw in the ingame console
		constexpr auto console_fade = "con_fade"; //the transparency of the backdrop for the console overlay
		constexpr auto console_level = "con_level"; //the logging level to display in the console

		//vid vars
		constexpr auto video_fullscreen = "vid_fullscreen"; // draw in fullscreen mode
		constexpr auto video_resizable = "vid_resizable"; // allow window to be freely resized by the user
		constexpr auto video_width = "vid_width"; // resolution width
		constexpr auto video_height = "vid_height"; // resolution height
		constexpr auto video_depth = "vid_depth"; // colour bit depth

		//server host and connection settings

		namespace default_value
		{
			constexpr auto render_threadcount = 0; // deprecated
			constexpr auto render_drawtime = 0.f;

			constexpr auto server_threadcount = 0; // deprecated

			constexpr auto client_tickrate = 30;
			constexpr auto client_avg_tick_time = 0.f;
			constexpr auto client_max_tick_time = 0.f;
			constexpr auto client_min_tick_time = 0.f;
			constexpr auto client_total_tick_time = 0.f;
			constexpr auto client_tick_max = 150;
			constexpr auto client_previous_frametime = -1.f;
			constexpr auto client_tick_count = 0;

			constexpr auto file_portable = false;
			constexpr auto file_deflate = true;

			constexpr auto console_charsize = 15;
			constexpr auto console_fade = 180;
			constexpr auto console_level = 2;

			constexpr auto video_fullscreen = false;
			constexpr auto video_resizable = false;
            constexpr int32 video_width = 800;
            constexpr int32 video_height = 600;
            constexpr int32 video_depth = 32;
		}
	}
}

#endif //!HADES_CONSOLEVARS_HPP
