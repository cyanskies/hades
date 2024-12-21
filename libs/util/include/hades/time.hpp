#ifndef HADES_UTIL_TIME_HPP
#define HADES_UTIL_TIME_HPP

#include <chrono>
#include <string>
#include <type_traits>

namespace hades
{
	using time_clock = std::conditional_t<std::chrono::high_resolution_clock::is_steady,
		std::chrono::high_resolution_clock, 
		std::chrono::steady_clock>;
	using time_point = time_clock::time_point;

	using wall_clock = std::chrono::system_clock;

	template<typename Rep, typename Period>
	using basic_duration = std::chrono::duration<Rep, Period>;

	using std::chrono::nanoseconds;
	using std::chrono::microseconds;
	using std::chrono::milliseconds;
	using milliseconds_float = basic_duration<float, milliseconds::period>;
	using std::chrono::seconds;
	using seconds_float = basic_duration<float, seconds::period>;

	// TODO: rename to duration?
	using time_duration = time_point::duration;

	float normalise_time(time_point, time_duration) noexcept;

	enum class duration_ratio {
		seconds,
		millis,
		micros,
		nanos
	};

	// TODO: doesn't matter, should deprecated the hades::string using anyway
	// NOTE: using std::string here, since pulling in string.hpp causes weird issues
	// 	if string.hpp is included before to_string(time_duration) is declared, 
	//  then other files have trouble finding to_string(duration) due to ADL;
	//	The fact that this workaround works might be a MSVC bug.

	// TODO: reimp using std::format
	std::pair<std::string, duration_ratio> duration_to_string(time_duration);
	// TODO: replace with pick_duration_ratio for use with std::format
	std::string duration_to_string(time_duration, duration_ratio, int units_after_decimal = 2);
	// TODO: reimp using std::format
	std::string to_string(time_duration); // NOTE: ADL lookup cannot find this overload
	time_duration duration_from_string(std::string_view) noexcept;
}

#endif // !HADES_UTIL_TIME_HPP
