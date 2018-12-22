#include "hades/vector_math.hpp"

#include <algorithm>
#include <cmath>
#include <type_traits>

#include "hades/utility.hpp"

namespace hades
{
	template<typename T>
	constexpr bool operator==(const vector_t<T> &lhs, const vector_t<T> &rhs)
	{
		return lhs.x == rhs.x && lhs.y == rhs.y;
	}

	template<typename T>
	constexpr bool operator!=(const vector_t<T> &lhs, const vector_t<T> &rhs)
	{
		return !(lhs == rhs);
	}

	template<typename T>
	constexpr vector_t<T> operator+(const vector_t<T> &lhs, const vector_t<T> &rhs)
	{
		return { lhs.x + rhs.x, lhs.y + rhs.y };
	}

	template<typename T>
	constexpr vector_t<T> operator-(const vector_t<T> &lhs, const vector_t<T> &rhs)
	{
		return { lhs.x - rhs.x, lhs.y - rhs.y };
	}

	template<typename T>
	constexpr vector_t<T> operator*(const vector_t<T> &lhs, T rhs)
	{
		return { lhs.x * rhs, lhs.y * rhs };
	}

	template<typename T>
	constexpr vector_t<T> operator/(const vector_t<T> &lhs, T rhs)
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
		constexpr T magnitude(vector_t<T> v)
		{
			const auto mag = std::hypot(v.x, v.y);
			//if T is not a floating point type then sqrt will return double
			if constexpr (std::is_same_v<decltype(mag), T>)
				return mag;
			else
				return static_cast<T>(mag);
		}

		template<typename T>
		constexpr T magnitude_squared(vector_t<T> v)
		{
			return dot(v, v);
		}

		template<typename T>
		constexpr T angle(vector_t<T> v)
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
		constexpr vector_t<T> resize(vector_t<T> v, T length)
		{
			const auto mag = magnitude(v);
			if (mag == 0)
				return v;

			return v * (length / mag);
		}

		template<typename T>
		constexpr vector_t<T> unit(vector_t<T> v)
		{
			const auto mag = magnitude(v);
			if (mag == 0)
				return vector_t<T>{};

			return v / mag;
		}

		template<typename T>
		constexpr T distance(vector_t<T> a, vector_t<T> b)
		{
			const auto ab = a - b;
			return magnitude(ab);
		}

		template<typename T>
		constexpr vector_t<T> reverse(vector_t<T> v)
		{
			return { -v.x, -v.y };
		}

		template<typename T>
		constexpr vector_t<T> abs(vector_t<T> v)
		{
			return { std::abs(v.x), std::abs(v.y) };
		}

		template<typename T>
		constexpr vector_t<T> perpendicular(vector_t<T> v)
		{
			return { -v.y, x };
		}

		template<typename T>
		constexpr vector_t<T> perpendicular_reverse(vector_t<T> v)
		{
			return { v.y, -v.x };
		}

		template<typename T>
		constexpr vector_t<T> clamp(vector_t<T> val, vector_t<T> min, vector_t<T> max)
		{
			const auto x = std::clamp(val.x, std::min(min.x, max.x), std::max(min.x, max.x));
			const auto y = std::clamp(val.y, std::min(min.y, max.y), std::max(min.y, max.y));
			return { x, y };
		}

		template<typename T>
		constexpr T dot(vector_t<T> a, vector_t<T> b)
		{
			return a.x * b.x + a.y * b.y;
		}

		template <typename T>
		constexpr vector_t<T> project(vector_t<T> vector, vector_t<T> axis)
		{
			assert(axis != vector_t<T>());
			return axis * (dot(vector, axis) / magnitude_squared(axis));
		}
	}
}