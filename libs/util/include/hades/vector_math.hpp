#ifndef HADES_UTIL_MATH_HPP
#define HADES_UTIL_MATH_HPP

#include <tuple>

namespace hades::vector
{
	//returns the length of the vector
	template<typename T>
	T length(T x, T y);

	//changes the length of a vector to match the provided length
	template<typename T>
	std::tuple<T, T> resize(T x, T y, T length);

	template<typename T>
	std::tuple<T, T> unit(T x, T y);

	//returns a vector that points 90 degrees of the origional vector
	template<typename T>
	std::tuple<T, T> purpendicular(T x, T y);

	//returns a vector that points 280 degrees of the origional vector
	template<typename T>
	std::tuple<T, T> purpendicular_reverse(T x, T y);

	template<typename T>
	std::tuple<T, T> clamp(T vx, T vy, T minx, T miny, T maxx, T maxy);
}

#include "hades/detail/vector_math.inl"

#endif // !HADES_UTIL_MATH_HPP