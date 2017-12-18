#ifndef HADES_UTILITY_HPP
#define HADES_UTILITY_HPP

#include <cassert>
#include <limits>
#include <random>
#include <sstream>

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

	//pass a back_inserter to result
	template<typename Out>
	void split(const std::string &s, char delim, Out result) {
		std::stringstream ss;
		ss.str(s);
		std::string item;
		while (std::getline(ss, item, delim)) {
			*(result++) = item;
		}
	}
}

#endif // !HADES_UTILITY_HPP