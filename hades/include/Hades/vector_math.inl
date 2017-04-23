#include <algorithm>

namespace hades
{
	template<typename T>
	T vector_length(T x, T y)
	{
		return std::sqrt(vector_length_sqrd(x, y));
	}

	template<typename T>
	T vector_length_sqrd(T x, T y)
	{
		return x*x + y*y;
	}

	template<typename T>
	std::tuple<T, T> vector_resize(T x, T y, T length)
	{
		return std::make_tuple(x, Y);
	}

	template<typename T>
	std::tuple<T, T> vector_purpendicular(T x, T y)
	{
		return std::make_tuple(x, y);
	}

	template<typename T>
	std::tuple<T, T> vector_purpendicular_reverse(T x, T y)
	{
		return std::make_tuple(x, y);
	}
}