#ifndef HADES_UTIL_RANDOM_HPP
#define HADES_UTIL_RANDOM_HPP

#include <iterator>
#include <random>

namespace hades::detail
{
	thread_local static inline auto random_generator = std::default_random_engine{ std::random_device{}() };
	
	// valid types for uniform_int_distribution
	// note the absence of char(uint8) and any of the extended character types
	// this is why we cant simply use std::integral
	template<typename T>
	concept uniform_int = std::same_as<T, short> ||
		std::same_as<T, int> ||
		std::same_as<T, long> ||
		std::same_as<T, long long> ||
		std::same_as<T, unsigned short> ||
		std::same_as<T, unsigned int> ||
		std::same_as<T, unsigned long> ||
		std::same_as<T, unsigned long long>;

	// as above but limited for uniform_real_distribution
	// the built in floating_point concept includes custom extended types, which real_distribution disallows
	template<typename T>
	concept uniform_real = std::same_as<T, float> ||
		std::same_as<T, double> ||
		std::same_as<T, long double>;

	template<uniform_int T>
	T random(T min, T max)
	{
		std::uniform_int_distribution<T> random{ min, max };
		return random(random_generator);
	}

	template<uniform_real T>
	T random(T min, T max)
	{
		std::uniform_real_distribution<T> random{ min, max };
		return random(random_generator);
	}
}

namespace hades
{
	template<typename T>
		requires detail::uniform_int<T> || detail::uniform_real<T>
	T random(T min, T max)
	{
		if (min > max)
			return detail::random(max, min);

		return detail::random(min, max);
	}

	inline bool random()
	{
		return random(unsigned short{ 0 }, unsigned short{ 1 }) != 0;
	}

	template<typename Iter>
	Iter random_element(Iter first, const Iter last)
	{
		if (first == last)
			return first;

		const auto dist = std::distance(first, last) - 1;
		const auto target = detail::random(std::decay_t<decltype(dist)>{}, dist);

		std::advance(first, target);
		return first;
	}
}

#endif // !HADES_UTIL_RANDOM_HPP
