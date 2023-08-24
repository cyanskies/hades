#include "hades/vector_math.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <numbers>
#include <tuple>
#include <type_traits>

#include "hades/math.hpp"
#include "hades/tuple.hpp"
#include "hades/utility.hpp"

namespace hades
{
	template<typename T, std::size_t Size>
	constexpr std::size_t vector_t<T, Size>::size() noexcept
	{
		return Size;
	}

	template<typename T, std::size_t Size>
	constexpr vector_t<T, Size>& vector_t<T, Size>::operator+=(const vector_t& rhs) noexcept
	{
		this->x += rhs.x;
		this->y += rhs.y;
		if constexpr (Size > 2)
			this->z += rhs.z;
		if constexpr (Size > 3)
			this->w += rhs.w;
		return *this;
	}

	template<typename T, std::size_t Size>
	constexpr vector_t<T, Size>& vector_t<T, Size>::operator-=(const vector_t& rhs) noexcept
	{
		this->x -= rhs.x;
		this->y -= rhs.y;
		if constexpr (Size > 2)
			this->z -= rhs.z;
		if constexpr (Size > 3)
			this->w -= rhs.w;
		return *this;
	}

	template<typename T, std::size_t Size>
	constexpr vector_t<T, Size>& vector_t<T, Size>::operator*=(const T rhs) noexcept
	{
		this->x *= rhs;
		this->y *= rhs;
		if constexpr (Size > 2)
			this->z *= rhs;
		if constexpr (Size > 3)
			this->w *= rhs;
		return *this;
	}

	template<typename T, std::size_t Size>
	constexpr vector_t<T, Size>& vector_t<T, Size>::operator/=(const T rhs) noexcept
	{
		this->x /= rhs;
		this->y /= rhs;
		if constexpr (Size > 2)
			this->z /= rhs;
		if constexpr (Size > 3)
			this->w /= rhs;
		return *this;
	}

	template<typename T, std::size_t Size>
	constexpr T& vector_t<T, Size>::operator[](std::size_t i) noexcept
	{
		assert(i < Size);
		if constexpr (Size < 3)
		{
			constexpr auto arr = std::array{ &vector_t::x, &vector_t::y };
			return std::invoke(arr[i], this);
		}
		else if constexpr (Size == 3)
		{
			constexpr auto arr = std::array{ &vector_t::x, &vector_t::y, &vector_t::z };
			return std::invoke(arr[i], this);
		}
		else
		{
			constexpr auto arr = std::array{ &vector_t::x, &vector_t::y, &vector_t::z, &vector_t::w };
			return std::invoke(arr[i], this);
		}
	}

	template<typename T, std::size_t Size>
	constexpr const T& vector_t<T, Size>::operator[](std::size_t i) const noexcept
	{
		assert(i < Size);
		if constexpr (Size < 3)
		{
			constexpr auto arr = std::array{ &vector_t::x, &vector_t::y };
			return std::invoke(arr[i], this);
		}
		else if constexpr (Size == 3)
		{
			constexpr auto arr = std::array{ &vector_t::x, &vector_t::y, &vector_t::z };
			return std::invoke(arr[i], this);
		}
		else
		{
			constexpr auto arr = std::array{ &vector_t::x, &vector_t::y, &vector_t::z, &vector_t::w };
			return std::invoke(arr[i], this);
		}
	}

	template<typename T, std::size_t Size>
	template<typename U>
	inline constexpr vector_t<T, Size>::operator vector_t<U, Size>() const noexcept
	{
		constexpr auto cast_func = [](auto val) {
			if constexpr (std::is_integral_v<T> && std::is_integral_v<U>)
				return integer_cast<U>(val);
			else if constexpr (std::is_integral_v<U>)
				return integral_cast<U>(val);
			else if constexpr (std::floating_point<U>)
				return float_cast<U>(val);
			else
				static_assert(always_false_v<T, U>, "invalid type");
			};

		if constexpr (Size < 3)
		{
			return vector_t<U>{
				cast_func(this->x),
				cast_func(this->y)
			};
		}
		else if constexpr (Size > 2)
		{
			return vector_t<U, Size>{
				cast_func(this->x),
				cast_func(this->y),
				cast_func(this->z)
			};
		}
		else if constexpr (Size > 3)
		{
			return vector_t<U, Size>{
				cast_func(this->x),
				cast_func(this->y),
				cast_func(this->z),
				cast_func(this->w)
			};
		}
	}

	template<typename T, std::size_t N>
	constexpr bool operator==(const vector_t<T, N> &lhs, const vector_t<T, N> &rhs) noexcept
	{
		auto ret = lhs.x == rhs.x && lhs.y == rhs.y;
		if constexpr (N > 2)
			ret = ret && lhs.z == rhs.z;
		if constexpr (N > 3)
			ret = ret && lhs.w == rhs.w;
		return ret;
	}

	template<typename T, std::size_t N>
	constexpr bool operator!=(const vector_t<T, N> &lhs, const vector_t<T, N> &rhs) noexcept
	{
		return !(lhs == rhs);
	}

	template<typename T>
	constexpr vector2<T> operator+(const vector2<T> &lhs, const vector2<T> &rhs) noexcept
	{
		return { lhs.x + rhs.x, lhs.y + rhs.y };
	}

	template<typename T>
	constexpr vector2<T> operator-(const vector2<T> &lhs, const vector2<T> &rhs) noexcept
	{
		return { lhs.x - rhs.x, lhs.y - rhs.y };
	}

	template<typename T, typename U, std::enable_if_t<std::is_convertible_v<U, T>, int>>
	constexpr vector2<T> operator*(const vector2<T> &lhs, U rhs) noexcept
	{
		return { lhs.x * rhs, lhs.y * rhs };
	}

	template<typename T>
	constexpr vector2<T> operator/(const vector2<T> &lhs, T rhs) noexcept
	{
		return { lhs.x / rhs, lhs.y / rhs };
	}

	template<typename T>
	vector2<T> to_vector(pol_vector_t<T> v)
	{
		return { vector::x_comp(v), vector::y_comp(v) };
	}

	template<typename T>
	pol_vector_t<T> to_rad_vector(vector2<T> v)
	{
		return { vector::angle(v), vector::magnitude(v) };
	}

	template<typename T, std::size_t N>
	string vector_to_string(vector_t<T, N> v)
	{
		using namespace std::string_view_literals;
		if constexpr (N < 3)
			return std::format("[{}, {}]"sv, v.x, v.y);
		else if constexpr(N == 3)
			return std::format("[{}, {}, {}]"sv, v.x, v.y, v.z);
		else if constexpr (N > 3)
			return std::format("[{}, {}, {}, {}]"sv, v.x, v.y, v.z, v.w);
	}

	namespace vector
	{
		template<typename T>
		T magnitude(vector2<T> v)
		{
			const auto mag = std::hypot(v.x, v.y);
			//hypot returns either float or double
			if constexpr (std::is_integral_v<T>)
				return integral_cast<T>(mag);
			else
				return float_cast<T>(mag);
		}

		template<typename T>
		constexpr T magnitude_squared(vector2<T> v) noexcept
		{
			return v.x*v.x + v.y*v.y;
		}

		template<typename T>
		auto angle(vector2<T> v) noexcept
		{
			//NOTE: y is inverted because opengl
			if constexpr (std::is_floating_point_v<T>)
				return to_degrees(std::atan2(v.y * -1.f, v.x));
			else
				return to_degrees(std::atan2(static_cast<float>(v.y) * -1.f,
					static_cast<float>(v.x)));
		}

		template<typename T>
		[[nodiscard]] auto angle(vector2<T> a, vector2<T> b) noexcept
		{
			const auto d = dot(a, b);
			const auto determinant = a.x * b.y - a.y * b.x;
			return std::atan2(determinant, d);
		}

		template<typename T>
		T x_comp(pol_vector_t<T> v) noexcept
		{
			return v.m * std::cos(v.a);
		}

		template<typename T>
		T y_comp(pol_vector_t<T> v) noexcept
		{
			return v.m * std::sin(v.a);
		}

		template<typename T>
		vector2<T> resize(vector2<T> v, T length) noexcept
		{
			return unit(v) * length;
		}

		template<typename T>
		vector2<T> unit(vector2<T> v) noexcept
		{
			const auto inverse_root = 1 / std::sqrt(magnitude_squared(v));
			return v * inverse_root;
		}

		template<typename T>
		T distance(vector2<T> a, vector2<T> b) noexcept
		{
			const auto ab = a - b;
			return magnitude(ab);
		}

		template<typename T>
		constexpr vector2<T> reverse(vector2<T> v) noexcept
		{
			return { -v.x, -v.y };
		}

		template<typename T>
		vector2<T> abs(vector2<T> v) noexcept
		{
			return { std::abs(v.x), std::abs(v.y) };
		}

		template<typename T>
		constexpr vector2<T> perpendicular(vector2<T> v) noexcept
		{
			return { -v.y, v.x };
		}

		template<typename T>
		constexpr vector2<T> perpendicular_reverse(vector2<T> v) noexcept
		{
			return { v.y, -v.x };
		}

		template<typename T>
		constexpr vector2<T> clamp(vector2<T> val, vector2<T> min, vector2<T> max) noexcept
		{
			const auto x = std::clamp(val.x, std::min(min.x, max.x), std::max(min.x, max.x));
			const auto y = std::clamp(val.y, std::min(min.y, max.y), std::max(min.y, max.y));
			return { x, y };
		}

		template<typename T>
		constexpr T dot(vector2<T> a, vector2<T> b) noexcept
		{
			return a.x * b.x + a.y * b.y;
		}

		template <typename T>
		constexpr vector2<T> project(vector2<T> vector, vector2<T> axis) noexcept
		{
			assert(axis != vector2<T>());
			return axis * (dot(vector, axis) / magnitude_squared(axis));
		}

		template<typename T>
		constexpr vector2<T> reflect(vector2<T> vector, vector2<T> normal) noexcept
		{
			const auto mult = 2.f * dot(vector, normal);
			const auto sub = normal * mult;
			return vector - sub;
		}
	}
}
