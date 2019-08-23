#include "hades/vector_math.hpp"

#include <algorithm>
#include <cmath>
#include <tuple>
#include <type_traits>

#include "hades/utility.hpp"

namespace hades::detail
{
	//TODO: compilers have flags fot fast vd precise floating point math
	//		we should toggle those and use std::sqrt rather than this tricky shit
	//NOTE: Fast Inverse Square Root
	// from: https://en.wikipedia.org/wiki/Fast_inverse_square_root
	template<typename Float32>
	Float32 frsqrt32(Float32 f) noexcept
	{
		static_assert(sizeof(Float32) == 4);

		constexpr auto magic_constant = 0x5F375A86;

		const auto f2 = f * 0.5f;
		const auto threehalfs = 1.5f;

		auto i = uint32_t{};
		std::memcpy(&i, &f, std::size_t{ 4 });
		i = magic_constant - (i >> 1);

		std::memcpy(&f, &i, std::size_t{ 4 });
		f *= (threehalfs - (f2 * f * f));
		return f;
	}

	template<typename Float64>
	Float64 frsqrt64(Float64 f) noexcept
	{
		static_assert(sizeof(Float64) == 8);

		constexpr auto magic_constant = 0x5FE6EB50C7B537A9;

		const auto f2 = f * 0.5f;
		const auto threehalfs = 1.5f;

		auto i = uint64_t{};
		std::memcpy(&i, &f, std::size_t{ 8 });
		i = magic_constant - (i >> 1);

		std::memcpy(&f, &i, std::size_t{ 8 });
		f *= (threehalfs - (f2 * f * f));
		return f;
	}
}

namespace hades
{
	template<typename Float,
		typename std::enable_if_t<std::is_floating_point_v<Float>&&
		std::numeric_limits<Float>::is_iec559, int>>
	Float frsqrt(Float f) noexcept
	{
		static_assert(sizeof(Float) == 4 ||
			sizeof(Float) == 8);

		if constexpr (sizeof(Float) == 4)
			return detail::frsqrt32(f);
		else
			return detail::frsqrt64(f);
	}

	/*template<typename T>
	inline constexpr vector_t<T>::vector_t(T x, T y) noexcept(std::is_nothrow_constructible_v<T, T>)
		: x{ x }, y{ y }
	{}

	template<typename T>
	inline constexpr vector_t<T>::vector_t(const std::pair<T, T> &p) noexcept(std::is_nothrow_constructible_v<T, T>)
		: x{ p.first }, y{ p.second}
	{}

	template<typename T>
	template<typename U, std::enable_if_t<detail::vector_t_good_tuple_v<T, U>, int>>
	constexpr vector_t<T>::vector_t(const U &u) noexcept(std::is_nothrow_constructible_v<T, T>)
		: x{std::get<0>(u)}, y{std::get<1>(u)}
	{
		static_assert(std::is_same_v<T, std::tuple_element_t<0, U>> && 
			std::is_same_v<T, std::tuple_element<1, U>> && 
			std::tuple_size_v<U> == 2u,
			"a tuple like type can only be used to contruct a vector_t "
			"if it contains two elements of the same type as vector_t::value_type");
	}

	template<typename T>
	template<typename U, std::enable_if_t<!is_tuple_v<U>, int>>
	constexpr vector_t<T>::vector_t(const U &u) noexcept(std::is_nothrow_constructible_v<T, T>)
		: x{ u.x }, y{ u.y }
	{
		static_assert(std::is_same_v<T, decltype(u.x)> && std::is_same_v<T, decltype(u.y)>,
			"x and y must both be the same type as vector_t::value_type");
	}*/

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
		T angle(vector_t<T> v) noexcept
		{
			return std::atan2(v.y, v.x);
		}

		template<typename T>
		constexpr T x_comp(rad_vector_t<T> v) noexcept
		{
			return v.m * std::cos(v.a);
		}

		template<typename T>
		constexpr T y_comp(rad_vector_t<T> v) noexcept
		{
			return v.m * std::sin(v.a);
		}

		template<typename T>
		vector_t<T> resize(vector_t<T> v, T length) noexcept
		{
			return unit(v) * length;
		}

		template<typename T>
		vector_t<T> unit(vector_t<T> v) noexcept
		{
			const auto inverse_root = frsqrt(static_cast<float>(magnitude_squared(v)));
			return v * inverse_root;
		}

		template<typename T>
		T distance(vector_t<T> a, vector_t<T> b) noexcept
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
		vector_t<T> abs(vector_t<T> v) noexcept
		{
			return { std::abs(v.x), std::abs(v.y) };
		}

		template<typename T>
		constexpr vector_t<T> perpendicular(vector_t<T> v) noexcept
		{
			return { -v.y, x };
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
	}
}