#include "hades/timers.hpp"

#include <algorithm>
#include <cassert>
#include <set>

namespace hades
{
	float normalise_time(time_point t, time_duration d)
	{
		const auto duration_point = time_point{ d };

		//reduce t untill is is less than one duration;
		while (t > duration_point)
			t -= d;

		return  t.time_since_epoch().count() / d.count();
	}

	time_duration duration_from_string(std::string_view s)
	{
		//TODO: accept differnt time scales?
		//using extensions?
		//easy way to parse these?
		using namespace std::string_view_literals;
		constexpr auto nano_ext = "ns"sv;
		constexpr auto micro_ext = "us"sv;
		constexpr auto milli_ext = "ms"sv;
		constexpr auto second_ext = "se"sv;

		//if string is too short for a extension
		//then assume millis
		if (s.size() < 3u)
		{
			const auto ms_count = from_string<milliseconds::rep>(s);
			return milliseconds{ ms_count };
		}

		const auto ext = s.substr(s.size() - 2, 2);

		if (ext == second_ext)
		{
			const auto second_count = from_string<seconds::rep>(s);
			return time_cast<time_duration>(seconds{ second_count });
		}
		else if (ext == nano_ext)
		{
			const auto nano_count = from_string<nanoseconds::rep>(s);
			return nanoseconds{ nano_count };
		}
		else if (ext == micro_ext)
		{
			const auto micro_count = from_string<microseconds::rep>(s);
			return microseconds{ micro_count };
		}
		else if (ext == milli_ext)
		{
			const auto milli_count = from_string<milliseconds::rep>(s);
			return milliseconds{ milli_count };
		}

		//default with no extention is ms
		const auto ms_count = from_string<milliseconds::rep>(s);
		return milliseconds{ ms_count };
	}

	constexpr types::int32 timer_system::invalid_timer = 0;

	types::int32 timer_system::createTimer(time_duration duration, bool repeating, std::function<bool(void)> &function)
	{
		time_point current;
		{
			std::lock_guard<std::mutex> wlock(_watch_mutex);
			current = _time;
		}

		time_event t{ function,  duration, time_point{}, current + duration, repeating, false };
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
