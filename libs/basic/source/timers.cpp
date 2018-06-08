#include "hades/timers.hpp"

#include <algorithm>
#include <cassert>
#include <set>

namespace hades
{
	const types::int32 timer_system::invalid_timer = 0;

	types::int32 timer_system::createTimer(time_duration duration, bool repeating, std::function<bool(void)> &function)
	{
		time_point current;
		{
			std::lock_guard<std::mutex> wlock(_watch_mutex);
			current = _time;
		}

		time_event t{ function, repeating, duration, false, time_point{}, current + duration };
		types::int32 id = ++_timerCount;

		std::lock_guard<std::mutex> tlock(_add_mutex);
		_add_list.insert(std::make_pair(id, t));

		return id;
	}

	void timer_system::dropTimer(types::int32 id)
	{
		std::lock_guard<std::mutex> rlock(_remove_mutex);
		_remove_list.insert(id);
	}

	void timer_system::dropAll()
	{
		{
			std::lock_guard<std::mutex> tlock(_timer_mutex);
			_timers.clear();
		}

		{
			std::lock_guard<std::mutex> tlock(_add_mutex);
			_add_list.clear();
		}
	}


	void timer_system::update(time_duration dtime)
	{
		time_point current;
		{
			std::lock_guard<std::mutex> wlock(_watch_mutex);
			current = _time += dtime;
		}

		timer_map t_map;
		{
			std::lock_guard<std::mutex> tlock(_timer_mutex);
			t_map = _timers;
		}

		for(auto iter = t_map.begin(); iter != t_map.end(); ++iter)
		{
			if(iter->second.target_tick < current && !iter->second.paused)
			{
				if (!iter->second.function())
				{
					std::lock_guard<std::mutex> rlock(_remove_mutex);
					_remove_list.insert(iter->first);
				}

				if (iter->second.repeating)
					restart(iter->first);
				else
				{
					std::lock_guard<std::mutex> rlock(_remove_mutex);
					_remove_list.insert(iter->first);
				}
			}

		}

		std::lock_guard<std::mutex> tlock(_timer_mutex);

		for(types::int32 i : _remove_list)
			_timers.erase(i);

		_remove_list.clear();

		//add all new timers
		for (auto p : _add_list)
			_timers.insert(p);

		_add_list.clear();
	}

	void timer_system::pause(types::int32 id)
	{
		std::lock_guard<std::mutex> lock(_timer_mutex);

		auto iter = _timers.find(id);
		if(iter != _timers.end())
		{
			iter->second.paused = true;
			iter->second.pause_time = iter->second.target_tick - iter->second.duration;
		}
	}

	void timer_system::resume(types::int32 id)
	{
		time_point current;
		{
			std::lock_guard<std::mutex> wlock(_watch_mutex);
			current = _time;
		}

		std::lock_guard<std::mutex> tlock(_timer_mutex);
		auto iter = _timers.find(id);
		if(iter != _timers.end())
		{
			iter->second.paused = false;
			const auto time_diff = current - iter->second.pause_time;
			iter->second.target_tick = current + time_diff;
		}
	}

	void timer_system::restart(types::int32 id)
	{
		time_point current;
		{
			std::lock_guard<std::mutex> wlock(_watch_mutex);
			current = _time;
		}

		std::lock_guard<std::mutex> tlock(_timer_mutex);
		auto iter = _timers.find(id);
		if(iter != _timers.end())
		{
			iter->second.paused = false;
			iter->second.target_tick = current + iter->second.duration;
		}
	}
}
