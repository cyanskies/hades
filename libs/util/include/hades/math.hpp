#ifndef HADES_UTIL_MATH_HPP
#define HADES_UTIL_MATH_HPP

#include "hades/types.hpp"
#include "hades/vector_math.hpp"

namespace hades
{
	template<typename T>
	using point_t = vector_t<T>;

	template<typename T>
	T clamp(T value, T min, T max);

	//returns the distance between two points
	template<typename T>
	T distance(point_t<T> a, point_t<T> b);

	template<typename T>
	bool is_within(T value, T min, T max);

	template<typename T>
	bool range_within(T first_min, T first_max, T second_min, T second_max);
}

#include "hades/detail/math.inl"

#endif // !HADES_UTIL_MATH_HPP
