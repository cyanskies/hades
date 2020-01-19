#ifndef HADES_UTIL_VECTOR_MATH_HPP
#define HADES_UTIL_VECTOR_MATH_HPP

#include <optional>
#include <tuple>
#include <type_traits>

#include "hades/types.hpp"
#include "hades/utility.hpp"

namespace hades
{
	// fast reciprocal square root
	//computes fast inverse square root
	//eg: 1/sqrt(x)
	//only valid for 32 and 64bit iec559 and ieee754
	template<typename Float,
		typename std::enable_if_t<std::is_floating_point_v<Float> &&
		std::numeric_limits<Float>::is_iec559, int> = 0>
	Float frsqrt(Float) noexcept;

	template<typename T>
	struct vector_t
	{
		using value_type = T;

		constexpr vector_t& operator+=(const vector_t& rhs) noexcept
		{
			x += rhs.x;
			y += rhs.y;
			return *this;
		}

		//scalar multiplication
		constexpr vector_t& operator*=(const T rhs) noexcept
		{
			x *= rhs;
			y *= rhs;
			return *this;
		}

		constexpr T& operator[](std::size_t i) noexcept
		{
			assert(i < 2);
			return (&x)[i];
		}

		template<typename U>
		explicit constexpr operator vector_t<U>() const noexcept;

		T x, y;
	};

	static_assert(std::is_trivially_constructible_v<vector_t<int32>>);
	static_assert(std::is_trivially_assignable_v<vector_t<int32>, vector_t<int32>>);
	static_assert(std::is_trivially_copyable_v<vector_t<int32>>);
	static_assert(std::is_trivial_v<vector_t<int32>>);

	template<typename T>
	struct lerpable<vector_t<T>> : public std::bool_constant<lerpable_v<T>> {};

	template<typename Float, typename std::enable_if_t<lerpable_v<vector_t<Float>>, int> = 0>
	constexpr vector_t<Float> lerp(vector_t<Float> a, vector_t<Float> b, Float t) noexcept
	{
		return {
			lerp(a.x, b.x, t),
			lerp(a.y, b.y, t)
		};
	}

	template<typename Float, template<typename> typename Vector, std::enable_if_t<std::is_floating_point_v<Float>, int> = 0>
	inline bool float_near_equal(Vector<Float> a, Vector<Float> b, int32 units_after_decimal = 2) noexcept
	{
		return float_near_equal(a.x, b.x) && float_near_equal(a.y, b.y);
	}

	template<typename T>
	constexpr bool operator==(const vector_t<T> &lhs, const vector_t<T> &rhs) noexcept;

	template<typename T>
	constexpr bool operator!=(const vector_t<T> &lhs, const vector_t<T> &rhs) noexcept;

	template<typename T>
	constexpr vector_t<T> operator+(const vector_t<T> &lhs, const vector_t<T> &rhs) noexcept;

	template<typename T>
	constexpr vector_t<T> operator-(const vector_t<T> &lhs, const vector_t<T> &rhs) noexcept;

	template<typename T>
	constexpr vector_t<T> operator*(const vector_t<T> &lhs, T rhs) noexcept;

	template<typename T>
	constexpr vector_t<T> operator/(const vector_t<T> &lhs, T rhs) noexcept;

	using vector_int = vector_t<int32>;
	using vector_float = vector_t<float>;

	//TODO: polar vector type
	template<typename T>
	struct pol_vector_t
	{
		T a, m;
	};

	template<typename T>
	vector_t<T> to_vector(pol_vector_t<T> v);

	template<typename T>
	pol_vector_t<T> to_rad_vector(vector_t<T>);

	namespace vector
	{
		//returns the length of the vector
		template<typename T>
		T magnitude(vector_t<T>) noexcept;

		template<typename T>
		constexpr T magnitude_squared(vector_t<T> v) noexcept;

		//returns the angle of the vector
		template<typename T>
		T angle(vector_t<T>) noexcept;

		template<typename T>
		constexpr T x_comp(pol_vector_t<T>) noexcept;

		template<typename T>
		constexpr T y_comp(pol_vector_t<T>) noexcept;

		//changes the length of a vector to match the provided length
		template<typename T>
		vector_t<T> resize(vector_t<T>, T length) noexcept;

		template<typename T>
		vector_t<T> unit(vector_t<T>) noexcept;

		template<typename T>
		T distance(vector_t<T>, vector_t<T>) noexcept;

		template<typename T>
		constexpr vector_t<T> reverse(vector_t<T>) noexcept;

		template<typename T>
		vector_t<T> abs(vector_t<T>) noexcept;

		//returns a vector that points 90 degrees of the origional vector
		template<typename T>
		constexpr vector_t<T> perpendicular(vector_t<T>) noexcept;

		//returns a vector that points 280 degrees of the origional vector
		template<typename T>
		constexpr vector_t<T> perpendicular_reverse(vector_t<T>) noexcept;

		template<typename T>
		constexpr vector_t<T> clamp(vector_t<T> value, vector_t<T> min, vector_t<T> max) noexcept;

		template<typename T>
		constexpr T dot(vector_t<T> a, vector_t<T> b) noexcept;

		template <typename T>
		constexpr vector_t<T> project(vector_t<T> vector, vector_t<T> axis) noexcept;

		template <typename T>
		constexpr vector_t<T> reflect(vector_t<T> vector, vector_t<T> normal) noexcept;
	}
}

#include "hades/detail/vector_math.inl"

#endif // !HADES_UTIL_VECTOR_MATH_HPP