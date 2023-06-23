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

	using nanoseconds = std::chrono::nanoseconds;
	using microseconds = std::chrono::microseconds;
	using milliseconds = std::chrono::milliseconds;
	using milliseconds_float = basic_duration<float, milliseconds::period>;
	using seconds = std::chrono::seconds;
	using seconds_float = basic_duration<float, seconds::period>;

	using time_duration = time_point::duration;

	template<typename T> struct is_duration : std::false_type {};
	template<typename Rep, typename Period>
	struct is_duration<basic_duration<Rep, Period>> : std::true_type {};
	template<typename T>
	constexpr auto is_duration_v = is_duration<T>::value;

	template<typename TargetDuration, typename Rep2, typename Period2,
		typename = std::enable_if_t<is_duration_v<TargetDuration>>>
		constexpr TargetDuration time_cast(const basic_duration<Rep2, Period2> & duration)
	{
		return std::chrono::duration_cast<TargetDuration>(duration);
	}

	float normalise_time(time_point, time_duration) noexcept;

	enum class duration_ratio {
		seconds,
		millis,
		micros,
		nanos
	};

	// NOTE: using std::string here, since pulling in string.hpp causes weird issues
	// 	if string.hpp is included before to_string(time_duration) is declared, 
	//  everything in curve_extra.cpp explodes 	 
	using string = std::string;
	std::pair<string, duration_ratio> duration_to_string(time_duration);
	string duration_to_string(time_duration, duration_ratio, int units_after_decimal = 2);
	string to_string(time_duration);
	time_duration duration_from_string(std::string_view) noexcept;
}

#endif // !HADES_UTIL_TIME_HPP
