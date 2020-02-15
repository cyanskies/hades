#include "hades/math.hpp"

#include <algorithm>

namespace hades
{
	template<typename T>
	T clamp(T value, T min, T max)
	{
		return std::clamp(value, std::min(min, max), std::max(min, max));
	}

	template<typename T>
	T distance(point_t<T> a, point_t<T> b)
	{
		const auto x = b.x - a.x;
		const auto y = b.y - a.y;
		return vector::magnitude({ x, y });
	}

	template<typename T>
	bool is_within(T value, T min, T max)
	{
		return value >= std::min(min, max) && value < std::max(min, max);
	}

	template<typename T>
	bool range_within(T first_min, T first_max, T second_min, T second_max)
	{
		return std::max(first_min, first_max) >= std::min(second_min, second_max)
			&& std::min(first_min, first_max) <= std::max(second_min, second_max);
	}
}