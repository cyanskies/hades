#include "hades/game_loop.hpp"

#include <functional>
#include <type_traits>

namespace hades
{
	struct game_loop_timing
	{
		// time that the previous frame was generated(end of game update step)
		time_point previous_frame_time;
		// time that last frame was drawn
		time_point previous_draw_time;
		time_duration accumulator = time_duration::zero();
	};

	template<typename OnTick, typename OnDraw, typename PerformanceStatistics>
	void game_loop(game_loop_timing &times, const time_duration dt, OnTick &&tick, OnDraw &&draw, PerformanceStatistics &stats)
	{
		static_assert(std::is_invocable_v<OnDraw, time_duration>,
			"OnDraw must accept a time_duration parameter, this represents the time between the last call to OnDraw and this call");
		constexpr auto use_stats = std::is_same_v<PerformanceStatistics, performance_statistics>;

		const auto new_time = time_clock::now();
		const auto frame_time = new_time - times.previous_frame_time;

		if constexpr (use_stats)
		{
			stats.frame_times.emplace_back(frame_time);
			constexpr auto max_sample_size = 120;

			// rolling average
			if (size(stats.frame_times) > max_sample_size)
			{
				const auto old_val = stats.frame_times.front();
				stats.frame_times.erase(begin(stats.frame_times));
				stats.average_frame_sum += frame_time - old_val;
				stats.average_frame_time = stats.average_frame_sum / max_sample_size;
			}
			else
			{
				stats.average_frame_sum += frame_time;
				stats.average_frame_time = stats.average_frame_sum / size(stats.frame_times);
			}
		}

		constexpr auto max_accumulator_overflow = time_cast<time_duration>(seconds_float{ 0.25f });
		const auto capped_frame_time = frame_time > max_accumulator_overflow ?
			max_accumulator_overflow : frame_time;

		times.accumulator += capped_frame_time;

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
		std::invoke(std::forward<OnDraw>(draw), draw_dt);

		times.previous_draw_time = time_clock::now();
		
		if constexpr (use_stats)
			stats.draw_duration = time_clock::now() - stats.draw_start;

		times.previous_frame_time = new_time;

		return;
	}
}