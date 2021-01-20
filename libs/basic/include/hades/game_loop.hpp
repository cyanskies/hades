#ifndef HADES_GAME_LOOP_HPP
#define HADES_GAME_LOOP_HPP

#include <vector>

#include "hades/time.hpp"

namespace hades
{
	struct game_loop_timing;

	struct performance_statistics
	{
		//the number of nanoseconds needed to generate the previous frame
		time_duration previous_frame_time{};
		//the start time of the update step
		time_point update_start{};
		//the duration of each call of OnTick
		std::vector<time_duration> tick_times{};
		//number of times OnTick was called
		std::size_t tick_count{};
		//used internally
		time_point tick_start{};
		//the time it took to perform all calls of OnTick
		time_duration update_duration{};
		//used internally
		time_point draw_start{};
		//the time it took to draw
		time_duration draw_duration{};
	};

	struct no_stats_t {};
	constexpr auto no_stats = no_stats_t{};

	// OnDraw call signiture: void OnDraw(time_duration dt);
	// OnDraw will be passed the time since the last draw call


	// game_loop_timing is needed to maintain timing calculations between iterations of the loop
	// dt is how much time passage each call to OnDraw represents, calculated by 1 second / desired_ticks_per_second
	// PerformanceStatistics is optional, will allow access to metrics related to the timings
	template<typename OnTick, typename OnDraw, typename PerformanceStatistics>
	void game_loop(game_loop_timing&, time_duration dt, OnTick&&, OnDraw&&, PerformanceStatistics& = no_stats);
}

#include "hades/detail/game_loop.inl"

#endif //!HADES_GAME_LOOP_HPP