#ifndef HADES_UTIL_RANDOM_HPP
#define HADES_UTIL_RANDOM_HPP

#include <iterator>
#include <random>
#include <type_traits>

namespace hades::detail
{
	std::default_random_engine& get_engine() noexcept;

	template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
	T random(T min, T max)
	{
		std::uniform_int_distribution<T> random{ min, max };
		return random(get_engine());
	}

	template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
	T random(T min, T max)
	{
		std::uniform_real_distribution<T> random{ min, max };
		return random(get_engine());
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
	typename Iter random_element(Iter first, const Iter last)
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
