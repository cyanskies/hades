#include "hades/vector_math.hpp"

#include <algorithm>

#include "hades/utility.hpp"

namespace hades
{
	template<typename T>
	vector_t<T> to_vector(rad_vector_t<T> v)
	{
		return { vector::x_comp(v), vector::y_comp(v) };
	}

	template<typename T>
	rad_vector_t<T> to_rad_vector(vector_t<T> v)
	{
		return { vector::angle(v), vector::magnitude(v) };
	}

	namespace vector
	{
		//TODO: many of these
		template<typename T>
		T magnitude(vector_t<T> v)
		{
			return std::sqrt(v.x * v.x + v.y * v.y);
		}

		template<typename T>
		vector_t<T> resize(vector_t<T> v, T length)
		{
			const auto unit_v = unit(v);
			return { unit_v.x * length, unit_v.y * length };
		}

		template<typename T>
		vector_t<T> unit(vector_t<T> v)
		{
			const auto mag = magnitude(v);
			return { v.x / mag, v.y / mag };
		}

		template<typename T>
		vector_t<T> perpendicular(vector_t<T> v)
		{
			return { -v.y, x };
		}

		template<typename T>
		vector_t<T> perpendicular_reverse(vector_t<T> v)
		{
			return { v.y, -v.x };
		}

		template<typename T>
		vector_t<T> clamp(vector_t<T> val, vector_t<T> min, vector_t<T> max)
		{
			const auto x = std::clamp(val.x, std::min(min.x, max.x), std::max(min.x, max.x));
			const auto y = std::clamp(val.y, std::min(min.y, max.y), std::max(min.y, max.y));
			return { x, y };
		}
	}
}