#ifndef HADES_UTILITY_HPP
#define HADES_UTILITY_HPP

#include <cassert>
#include <limits>
#include <random>

namespace hades {
	namespace
	{
		std::random_device rd;
		std::default_random_engine random_generator(rd());
	}

	inline bool floatEqual(float a, float b)
	{
		return fabs(a - b) <= std::numeric_limits<float>::epsilon() * 4;
	}

	template<typename T>
	T random(T min, T max)
	{
		if (min == max)
			return min;
		else if (min > max)
			return random(max, min);

		std::uniform_int_distribution<T> random(min, max);

		return random(random_generator);
	}

	inline bool random()
	{
		return random(0, 1) != 0;
	}

	template<class T>
	T clamp(T value, T max, T min)
	{
		if (value > max)
			return max;
		else if (value < min)
			return min;
		else
			return value;
	}
}

#endif // !HADES_UTILITY_HPP