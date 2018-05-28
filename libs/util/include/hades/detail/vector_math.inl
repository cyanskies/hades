#include "hades/vector_math.hpp"

#include <algorithm>

#include "hades/utility.hpp"

namespace hades::vector
{
	namespace detail
	{
		template<typename T>
		T length_sqrd(T x, T y)
		{
			return x * x + y * y;
		}
	}

	//TODO: many of these
	template<typename T>
	T length(T x, T y)
	{
		return std::sqrt(detail::length_sqrd(x, y));
	}

	template<typename T>
	std::tuple<T, T> resize(T x, T y, T length)
	{
		return std::make_tuple(x, y);
	}

	template<typename T>
	std::tuple<T, T> unit(T x, T y)
	{
		return resize(x, y, 1.f);
	}

	template<typename T>
	std::tuple<T, T> purpendicular(T x, T y)
	{
		return std::make_tuple(x, y);
	}

	template<typename T>
	std::tuple<T, T> purpendicular_reverse(T x, T y)
	{
		return std::make_tuple(x, y);
	}

	template<typename T>
	std::tuple<T, T> clamp(T vx, T vy, T minx, T miny, T maxx, T maxy)
	{
		return std::make_tuple(std::clamp(vx, minx, maxx), std::clamp(vy, miny, maxy));
	}
}