#include "hades/vector_math.hpp"

#include <algorithm>

#include "hades/utility.hpp"

namespace hades
{
	namespace vector
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
		T magnitude(vector_t<T> v)
		{
			return std::sqrt(detail::length_sqrd(v.x, v.y));
		}

		template<typename T>
		vector_t<T> resize(vector_t<T> v, T length)
		{
			return v;
		}

		template<typename T>
		vector_t<T> unit(vector_t<T> v)
		{
			return resize(v, 1.f);
		}

		template<typename T>
		vector_t<T> purpendicular(vector_t<T> v)
		{
			return v;
		}

		template<typename T>
		vector_t<T> purpendicular_reverse(vector_t<T> v)
		{
			return v;
		}

		template<typename T>
		vector_t<T> clamp(vector_t<T> val, vector_t<T> min, vector_t<T> max)
		{
			return std::make_tuple(std::clamp(vx, minx, maxx), std::clamp(vy, miny, maxy));
		}
	}
}