#include "Hades/TimerManager.hpp"

#include <algorithm>
#include <cassert>
#include <set>

namespace hades
{
	const int TimerManager::NO_TIMER = 0;

	TimerManager::TimerManager() : _timerCount(0) {};

	int TimerManager::createTimer(sf::Time duration, bool repeating, std::function<bool(void)> &function)
	{
		sf::Time currentTick;
		{
			std::lock_guard<std::mutex> wlock(_watchMutex);
			currentTick = _time;
		}

		TimeEvent t = {function, repeating, duration, false, sf::Time(), currentTick + duration};
		int id = ++_timerCount;

		std::lock_guard<std::mutex> tlock(_addMutex);
		_add_list.insert(std::make_pair(id, t));

		return id;
	}

	void TimerManager::dropTimer(int id)
	{
		std::lock_guard<std::mutex> rlock(_removeMutex);
		_remove_list.insert(id);
	}

	void TimerManager::dropAll()
	{
		{
			std::lock_guard<std::mutex> tlock(_timerMutex);
			_timers.clear();
		}

		{
			std::lock_guard<std::mutex> tlock(_addMutex);
			_add_list.clear();
		}
	}


	void TimerManager::update(sf::Time dtime)
	{
		sf::Time currentTick;
		{
			std::lock_guard<std::mutex> wlock(_watchMutex);
			currentTick = _time += dtime;
		}

		timer_map t_map;
		{
			std::lock_guard<std::mutex> tlock(_timerMutex);
			t_map = _timers;
		}

		for(auto iter = t_map.begin(); iter != t_map.end(); ++iter)
		{
			if(iter->second.targetTick < currentTick && !iter->second.paused)
			{
				if (!iter->second.function())
				{
					std::lock_guard<std::mutex> rlock(_removeMutex);
					_remove_list.insert(iter->first);
				}

				if (iter->second.repeating)
					restart(iter->first);
				else
				{
					std::lock_guard<std::mutex> rlock(_removeMutex);
					_remove_list.insert(iter->first);
				}
			}

		}

		std::lock_guard<std::mutex> tlock(_timerMutex);

		for(int i : _remove_list)
			_timers.erase(i);

		_remove_list.clear();

		//add all new timers
		for (auto p : _add_list)
			_timers.insert(p);

		_add_list.clear();
	}

	void TimerManager::pause(int id)
	{
		std::lock_guard<std::mutex> lock(_timerMutex);

		auto iter = _timers.find(id);
		if(iter != _timers.end())
		{
			iter->second.paused = true;
			iter->second.pauseTime = iter->second.targetTick - iter->second.duration;
		}
	}

	void TimerManager::resume(int id)
	{
		sf::Time currentTick;
		{
			std::lock_guard<std::mutex> wlock(_watchMutex);
			currentTick = _time;
		}

		std::lock_guard<std::mutex> tlock(_timerMutex);
		auto iter = _timers.find(id);
		if(iter != _timers.end())
		{
			iter->second.paused = false;
			iter->second.targetTick = currentTick + iter->second.pauseTime;
		}
	}

	void TimerManager::restart(int id)
	{
		sf::Time currentTick;
		{
			std::lock_guard<std::mutex> wlock(_watchMutex);
			currentTick = _time;
		}

		std::lock_guard<std::mutex> tlock(_timerMutex);
		auto iter = _timers.find(id);
		if(iter != _timers.end())
		{
			iter->second.paused = false;
			iter->second.targetTick = currentTick + iter->second.duration;
		}
	}
}
