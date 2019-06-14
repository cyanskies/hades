#ifndef HADES_UTIL_TIME_HPP
#define HADES_UTIL_TIME_HPP

#include <chrono>
#include <string_view>
#include <type_traits>

namespace hades
{
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
		constexpr TargetDuration time_cast(const basic_duration<Rep2, Period2> & duration)
	{
		return std::chrono::duration_cast<TargetDuration>(duration);
	}

	float normalise_time(time_point, time_duration) noexcept;

	//TODO: to_string(time_point), to_string(duration)

	time_duration duration_from_string(std::string_view) noexcept;
}
#endif // !HADES_UTIL_TIME_HPP