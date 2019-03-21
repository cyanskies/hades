#ifndef HADES_TIMERMANAGER_HPP
#define HADES_TIMERMANAGER_HPP

#include <chrono>
#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>

#include "hades/types.hpp"
#include "hades/utility.hpp"

namespace hades
{
	//TODO: add a timers.inl for all these function defs
	using time_clock = std::chrono::high_resolution_clock;
	using time_point = std::chrono::high_resolution_clock::time_point;

	template<typename Rep, typename Period>
	using basic_duration = std::chrono::duration<Rep, Period>;
	template<typename Rep>
	using basic_second = std::chrono::duration<Rep, std::chrono::seconds::period>;

	using nanoseconds = std::chrono::nanoseconds;
	using microseconds = std::chrono::microseconds;
	using milliseconds = std::chrono::milliseconds;
	using seconds = std::chrono::seconds;
	using seconds_float = basic_second<float>;

	using time_duration = time_point::duration;

	template<typename T> struct is_duration : std::false_type {};
	template<typename Rep, typename Period>
	struct is_duration<basic_duration<Rep, Period>> : std::true_type {};
	template<typename T>
	constexpr auto is_duration_v = is_duration<T>::value;

	template<typename TargetDuration, typename Rep2, typename Period2,
		typename = std::enable_if_t<is_duration_v<TargetDuration>>>
	constexpr TargetDuration time_cast(const basic_duration<Rep2, Period2> &duration)
	{
		return std::chrono::duration_cast<TargetDuration>(duration);
	}

	float normalise_time(time_point, time_duration);

	time_duration duration_from_string(std::string_view);

	//Thread safe.
	class timer_system
	{
	public:
		types::int32 createTimer(time_duration duration, bool repeating, std::function<bool(void)> &function);	
		void dropTimer(types::int32 id);
		void dropAll();

		// Controls for all timers.
		void update(time_duration dtime);

		// Controls for a specific timer, using an identifier.
		void pause(types::int32 id);
		void resume(types::int32 id);
		void restart(types::int32 id);

		static const types::int32 invalid_timer;

	private:

		struct time_event
		{
			std::function<bool(void)> function;
			time_duration duration; //how long the time is set for, for repeating and so on.
			time_point pause_time; // time between the pause and the finish.
			time_point target_tick;
			bool repeating;
			bool paused;
		};

		time_point _time = time_clock::now(); //total game watch

		std::mutex _timer_mutex, //guards the timer_map
			_remove_mutex, //guards the timer _remove_list
			_add_mutex, //guards the timer _add_list
			_watch_mutex; //guards the total _time variable

		using timer_map = std::unordered_map<types::int32, time_event>;
		timer_map _timers,
			_add_list; // add list stores timers before they are merged into the full timer list
		std::set<types::int32> _remove_list; // stores the timers to be killed before they are pulled from the main list
		types::int32 _timerCount;
	};
}

#endif // HADES_TIMERMANAGER_HPP
