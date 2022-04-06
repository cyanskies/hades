#ifndef HADES_TIMERMANAGER_HPP
#define HADES_TIMERMANAGER_HPP

#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <unordered_map>

#include "hades/time.hpp"
#include "hades/types.hpp"

namespace hades
{
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
		// TODO: review usage of set
		std::set<types::int32> _remove_list; // stores the timers to be killed before they are pulled from the main list
		types::int32 _timerCount;
	};
}

#endif // HADES_TIMERMANAGER_HPP
