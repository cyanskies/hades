#ifndef HADES_TIMERMANAGER_HPP
#define HADES_TIMERMANAGER_HPP

#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>

#include "SFML/System/Time.hpp"

namespace hades
{
	//Thread safe.
	class TimerManager
	{
	public:
		TimerManager();

		int createTimer(sf::Time duration, bool repeating, std::function<bool(void)> &function);	
		void dropTimer(int id);
		void dropAll();

		// Controls for all timers.
		void update(sf::Time dtime);

		// Controls for a specific timer, using an identifier.
		void pause(int id);
		void resume(int id);
		void restart(int id);

		static const int NO_TIMER;

	private:

		struct TimeEvent
		{
			std::function<bool(void)> function;
			bool repeating;
			sf::Time duration; //how long the time is set for, for repeating and so on.
			bool paused;
			sf::Time pauseTime; // time between the pause and the finish.
			sf::Time targetTick;
		};

		int _timerCount;
		sf::Time _time; //total game watch

		std::mutex _timerMutex, //guards the timer_map
			_removeMutex, //guards the timer _remove_list
			_addMutex, //guards the timer _add_list
			_watchMutex; //guards the total _time variable

		using timer_map = std::unordered_map<int, TimeEvent>;
		timer_map _timers,
			_add_list; // add list stores timers before they are merged into the full timer list
		std::set<int> _remove_list; // stores the timers to be killed before they are pulled from the main list
	};
}

#endif // HADES_TIMERMANAGER_HPP
