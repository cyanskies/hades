#include "hades/vector_math.hpp"

#include <algorithm>
#include <cmath>
#include <tuple>
#include <type_traits>

#include "hades/utility.hpp"

namespace hades::detail
{
	//TODO: compilers have flags for fast amd precise floating point math
	//		we should toggle those and use std::sqrt rather than this tricky shit
	//NOTE: Fast Inverse Square Root
	// from: https://en.wikipedia.org/wiki/Fast_inverse_square_root
	template<typename Float32>
	constexpr Float32 frsqrt32(Float32 f) noexcept
	{
		static_assert(std::is_trivially_copyable_v<Float32>);
		static_assert(sizeof(Float32) == 4);

		constexpr auto magic_constant = 0x5F375A86;

		const auto f2 = f * 0.5f;
		constexpr auto threehalfs = 1.5f;

		uint32_t i;
		static_assert(sizeof(Float32) == sizeof(uint32_t));
		std::memcpy(&i, &f, sizeof(Float32)); //std::bit_cast in c++20
		i = magic_constant - (i >> 1);

		std::memcpy(&f, &i, sizeof(Float32));
		f *= (threehalfs - (f2 * f * f));
		return f;
	}

	template<typename Float64>
	constexpr Float64 frsqrt64(Float64 f) noexcept
	{
		static_assert(std::is_trivially_copyable_v<Float64>);
		static_assert(sizeof(Float64) == 8);

		constexpr auto magic_constant = 0x5FE6EB50C7B537A9;

		const auto f2 = f * 0.5f;
		constexpr auto threehalfs = 1.5f;

		uint64_t i;
		static_assert(sizeof(Float64) == sizeof(uint64_t));
		std::memcpy(&i, &f, sizeof(Float64));
		i = magic_constant - (i >> 1);

		std::memcpy(&f, &i, sizeof(Float64));
		f *= (threehalfs - (f2 * f * f));
		return f;
	}
}

namespace hades
{
	template<typename Float,
		typename std::enable_if_t<std::is_floating_point_v<Float>&&
		std::numeric_limits<Float>::is_iec559, int>>
	constexpr Float frsqrt(Float f) noexcept
	{
		static_assert(sizeof(Float) == 4 ||
			sizeof(Float) == 8);

		if constexpr (sizeof(Float) == 4)
			return detail::frsqrt32(f);
		else
			return detail::frsqrt64(f);
	}

	template<typename T>
	template<typename U>
	inline constexpr vector_t<T>::operator vector_t<U>() const noexcept
	{
		if constexpr (std::is_integral_v<T>&& std::is_integral_v<U>)
		{
			return {
				integer_cast<U>(x),
				integer_cast<U>(y)
			};
		}
		else
		{
			//probably one of them is a float
			return {
				static_cast<U>(x),
				static_cast<U>(y)
			};
		}
	}

	template<typename T>
	constexpr bool operator==(const vector_t<T> &lhs, const vector_t<T> &rhs) noexcept
	{
		return lhs.x == rhs.x && lhs.y == rhs.y;
	}

	template<typename T>
	constexpr bool operator!=(const vector_t<T> &lhs, const vector_t<T> &rhs) noexcept
	{
		return !(lhs == rhs);
	}

	template<typename T>
	constexpr vector_t<T> operator+(const vector_t<T> &lhs, const vector_t<T> &rhs) noexcept
	{
		return { lhs.x + rhs.x, lhs.y + rhs.y };
	}

	template<typename T>
	constexpr vector_t<T> operator-(const vector_t<T> &lhs, const vector_t<T> &rhs) noexcept
	{
		return { lhs.x - rhs.x, lhs.y - rhs.y };
	}

	template<typename T>
	constexpr vector_t<T> operator*(const vector_t<T> &lhs, T rhs) noexcept
	{
		return { lhs.x * rhs, lhs.y * rhs };
	}

	template<typename T>
	constexpr vector_t<T> operator/(const vector_t<T> &lhs, T rhs) noexcept
	{
		return { lhs.x / rhs, lhs.y / rhs };
	}

	template<typename T>
	vector_t<T> to_vector(pol_vector_t<T> v)
	{
		return { vector::x_comp(v), vector::y_comp(v) };
	}

	template<typename T>
	pol_vector_t<T> to_rad_vector(vector_t<T> v)
	{
		return { vector::angle(v), vector::magnitude(v) };
	}

	template<typename T>
	string vector_to_string(vector_t<T> v)
	{
		using namespace std::string_literals;
		return "["s + to_string(v.x) + ", "s + to_string(v.y) + "]"s;
	}

	namespace vector
	{
		template<typename T>
		T magnitude(vector_t<T> v) noexcept
		{
			const auto mag = std::sqrt(magnitude_squared(v));
			//if T is not a floating point type then sqrt will return double
			if constexpr (std::is_same_v<decltype(mag), T>)
				return mag;
			else
				return static_cast<T>(mag);
		}

		template<typename T>
		constexpr T magnitude_squared(vector_t<T> v) noexcept
		{
			return v.x*v.x + v.y*v.y;
		}

		template<typename T>
		constexpr T angle(vector_t<T> v) noexcept
		{
			auto ret = T{};
			//NOTE: y is inverted because opengl
			if constexpr (std::is_floating_point_v<T>)
			{
				constexpr auto pi = T{ 3.1415926535897 }; //C++20 brings math constants finally ::rollseyes::
				ret = std::atan2(v.y * -1.f, v.x) * 180 / pi;
			}
			else
			{
				constexpr auto pi = float{ 3.1415926535897 };
				ret = static_cast<T>(std::atan2(static_cast<float>(v.y) * -1.f, static_cast<float>(v.x)) * 180 / pi);
			}

			//atan2 reurns in the range {180, -180}
			// convert to {0, 360}
			if (ret < 0)
				ret += 360;

			return ret;
		}

		template<typename T>
		constexpr T x_comp(pol_vector_t<T> v) noexcept
		{
			return v.m * std::cos(v.a);
		}

		template<typename T>
		constexpr T y_comp(pol_vector_t<T> v) noexcept
		{
			return v.m * std::sin(v.a);
		}

		template<typename T>
		vector_t<T> resize(vector_t<T> v, T length) noexcept
		{
			return unit(v) * length;
		}

		template<typename T>
		constexpr vector_t<T> unit(vector_t<T> v) noexcept
		{
			const auto inverse_root = frsqrt(static_cast<float>(magnitude_squared(v)));
			return v * inverse_root;
		}

		template<typename T>
		constexpr T distance(vector_t<T> a, vector_t<T> b) noexcept
		{
			const auto ab = a - b;
			return magnitude(ab);
		}

		template<typename T>
		constexpr vector_t<T> reverse(vector_t<T> v) noexcept
		{
			return { -v.x, -v.y };
		}

		template<typename T>
		constexpr vector_t<T> abs(vector_t<T> v) noexcept
		{
			return { std::abs(v.x), std::abs(v.y) };
		}

		template<typename T>
		constexpr vector_t<T> perpendicular(vector_t<T> v) noexcept
		{
			return { -v.y, v.x };
		}

		template<typename T>
		constexpr vector_t<T> perpendicular_reverse(vector_t<T> v) noexcept
		{
			return { v.y, -v.x };
		}

		template<typename T>
		constexpr vector_t<T> clamp(vector_t<T> val, vector_t<T> min, vector_t<T> max) noexcept
		{
			const auto x = std::clamp(val.x, std::min(min.x, max.x), std::max(min.x, max.x));
			const auto y = std::clamp(val.y, std::min(min.y, max.y), std::max(min.y, max.y));
			return { x, y };
		}

		template<typename T>
		constexpr T dot(vector_t<T> a, vector_t<T> b) noexcept
		{
			return a.x * b.x + a.y * b.y;
		}

		template <typename T>
		constexpr vector_t<T> project(vector_t<T> vector, vector_t<T> axis) noexcept
		{
			assert(axis != vector_t<T>());
			return axis * (dot(vector, axis) / magnitude_squared(axis));
		}

		template<typename T>
		constexpr vector_t<T> reflect(vector_t<T> vector, vector_t<T> normal) noexcept
		{
			const auto mult = 2.f * dot(vector, normal);
			const auto sub = normal * mult;
			return vector - sub;
		}
	}
}