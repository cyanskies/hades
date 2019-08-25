#include "hades/game_loop.hpp"

#include <type_traits>

namespace hades
{
	struct game_loop_timing
	{
		time_point previous_frame_time{};
		time_point previous_draw_time{};
		time_duration accumulator{};
	};

	template<typename OnTick, typename OnDraw, typename PerformanceStatistics>
	void game_loop(game_loop_timing &times, time_duration dt, OnTick tick, OnDraw draw, PerformanceStatistics &stats)
	{
		static_assert(std::is_invocable_v<OnDraw, time_duration>,
			"OnDraw must accept a time_duration parameter, this represents the time between the last call to OnDraw and this call");
		constexpr auto use_stats = std::is_same_v<PerformanceStatistics, performance_statistics>;

		const auto new_time = time_clock::now();
		auto frame_time = new_time - times.previous_frame_time;

		//store framerate
		if constexpr (use_stats)
			stats.previous_frame_time = frame_time;

		constexpr auto max_accumulator_overflow = time_cast<time_duration>(seconds_float{ 0.25f });
		if (frame_time > max_accumulator_overflow)
			frame_time = max_accumulator_overflow;

		times.accumulator += frame_time;

		if constexpr (use_stats)
		{
			stats.update_start = time_clock::now();
			stats.tick_times.clear();
			stats.tick_count = 0;
		}

		//perform additional logic updates if we're behind on logic
		while (times.accumulator >= dt)
		{
			if constexpr(use_stats)
				stats.tick_start = time_clock::now();

			std::invoke(tick);
			times.accumulator -= dt;

			if constexpr (use_stats)
			{
				stats.tick_times.emplace_back(time_clock::now() - stats.tick_start);
				++stats.tick_count;
			}
		}

		if constexpr (use_stats)
		{
			stats.draw_start = time_clock::now(); 
			stats.update_duration = stats.draw_start - stats.update_start;
		}
		
		const auto draw_dt = time_clock::now() - times.previous_draw_time;
		std::invoke(draw, draw_dt);

		times.previous_draw_time = time_clock::now();
		
		if constexpr (use_stats)
			stats.draw_duration = time_clock::now() - stats.draw_start;

		times.previous_frame_time = new_time;

		return;
	}
}