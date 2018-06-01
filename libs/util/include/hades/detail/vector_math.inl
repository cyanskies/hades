#include "hades/vector_math.hpp"

#include <algorithm>

#include "hades/utility.hpp"

namespace hades
{
	template<typename T>
	bool operator==(const vector_t<T> &lhs, const vector_t<T> &rhs)
	{
		return lhs.x == rhs.x && lhs.y == rhs.y;
	}

	template<typename T>
	bool operator!=(const vector_t<T> &lhs, const vector_t<T> &rhs)
	{
		return !(lhs == rhs);
	}

	template<typename T>
	vector_t<T> operator+(const vector_t<T> &lhs, const vector_t<T> &rhs)
	{
		return { lhs.x + rhs.x, lhs.y + rhs.y };
	}

	template<typename T>
	vector_t<T> operator-(const vector_t<T> &lhs, const vector_t<T> &rhs)
	{
		return { lhs.x - rhs.x, lhs.y - rhs.y };
	}

	template<typename T>
	vector_t<T> operator*(const vector_t<T> &lhs, T rhs)
	{
		return { lhs.x * rhs, lhs.y * rhs };
	}

	template<typename T>
	vector_t<T> operator/(const vector_t<T> &lhs, T rhs)
	{
		return { lhs.x / rhs, lhs.y / rhs };
	}

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
		template<typename T>
		T magnitude(vector_t<T> v)
		{
			return std::sqrt(magnitude_squared(v));
		}

		template<typename T>
		T magnitude_squared(vector_t<T> v)
		{
			return dot(v, v);
		}

		template<typename T>
		T angle(vector_t<T> v)
		{
			return std::atan2(v.y, v.x);
		}

		template<typename T>
		T x_comp(rad_vector_t<T> v)
		{
			return v.m * std::cos(v.a);
		}

		template<typename T>
		T y_comp(rad_vector_t<T> v)
		{
			return v.m * ::sin(v.a);
		}

		template<typename T>
		vector_t<T> resize(vector_t<T> v, T length)
		{
			const auto mag = magnitude(v);
			if (mag == 0)
				return v;

			return v * (length / mag);
		}

		template<typename T>
		vector_t<T> unit(vector_t<T> v)
		{
			const auto mag = magnitude(v);
			if (mag == 0)
				return vector_t<T>{};

			return v / mag;
		}

		template<typename T>
		T distance(vector_t<T> a, vector_t<T> b)
		{
			const auto ab = a - b;
			return magnitude(ab);
		}

		template<typename T>
		vector_t<T> reverse(vector_t<T> v)
		{
			return { -v.x, -v.y };
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

		template<typename T>
		T dot(vector_t<T> a, vector_t<T> b)
		{
			return a.x * b.x + a.y * b.y;
		}
	}
}