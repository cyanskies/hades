#ifndef HADES_UTIL_RANDOM_HPP
#define HADES_UTIL_RANDOM_HPP

#include <iterator>
#include <random>

namespace hades::detail
{
	thread_local static inline auto random_generator = std::default_random_engine{ std::random_device{}() };
	
	template<std::integral T>
	T random(T min, T max)
	{
		std::uniform_int_distribution<T> random{ min, max };
		return random(random_generator);
	}

	template<std::floating_point T>
	T random(T min, T max)
	{
		std::uniform_real_distribution<T> random{ min, max };
		return random(random_generator);
	}
}

namespace hades
{
	template<typename T>
	T random(T min, T max)
	{
		if (min > max)
			return detail::random(max, min);

		return detail::random(min, max);
	}

	inline bool random()
	{
		return random(0, 1) != 0;
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
