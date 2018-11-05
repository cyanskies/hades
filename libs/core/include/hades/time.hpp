#ifndef HADES_TIME_HPP
#define HADES_TIME_HPP

#include "SFML/System/Time.hpp"

#include "hades/timers.hpp"

//provides conversions from sfml time objects to standard times

namespace hades
{
	inline time_duration to_standard_time(sf::Time t)
	{
		const int64 micros = t.asMicroseconds();
		return microseconds{ micros };
	}

	inline time_point to_standard_time_point(sf::Time t)
	{
		return time_point{ to_standard_time(t) };
	}

	template<typename Rep, typename Period>
	inline sf::Time to_sfml_time(basic_duration<Rep, Period> t)
	{
		const auto micro_duration = std::chrono::duration_cast<microseconds>(t);
		const int64 micros = micro_duration.count();
		return sf::microseconds(micros);
	}

	inline sf::Time to_sfml_time(time_point t)
	{
		const auto nanos = t.time_since_epoch();
		const auto micro_duration = std::chrono::duration_cast<microseconds>(nanos);
		const int64 micros = micro_duration.count();
		return sf::microseconds(micros);
	}
}

#endif // !HADES_TIME_HPP
