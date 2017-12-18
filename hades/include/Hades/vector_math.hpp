#ifndef HADES_VECTOR_MATH_HPP
#define HADES_VECTOR_MATH_HPP

#include <tuple>

namespace hades
{
	//returns the length of the vector
	template<typename T>
	T vector_length(T x, T y);

	//returns the squared length of the vector
	template<typename T>
	T vector_length_sqrd(T x, T y);

	//changes the length of a vector to match the provided length
	template<typename T>
	std::tuple<T, T> vector_resize(T x, T y, T length);

	//returns a vector that points 90 degrees of the origional vector
	template<typename T>
	std::tuple<T, T> vector_purpendicular(T x, T y);

	//returns a vector that points 280 degrees of the origional vector
	template<typename T>
	std::tuple<T, T> vector_purpendicular_reverse(T x, T y);

	template<typename T>
	std::tuple<T, T> vector_clamp(T vx, T vy, T minx, T miny, T maxx, T maxy);
}

#include "Hades/vector_math.inl"

#endif // !HADES_VECTOR_MATH_HPP