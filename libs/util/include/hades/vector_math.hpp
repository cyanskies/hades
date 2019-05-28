#ifndef HADES_UTIL_VECTOR_MATH_HPP
#define HADES_UTIL_VECTOR_MATH_HPP

#include <optional>
#include <tuple>
#include <type_traits>

#include "hades/types.hpp"

namespace hades::detail
{
	template<typename T, typename U>
	constexpr bool vector_t_good_tuple_f() noexcept
	{
		if constexpr (is_tuple_v<U>)
		{
			return std::tuple_size_v<U> == 2 &&
				std::tuple_element_t<0u, U> == T &&
				std::tuple_element_t<1u, U> == T;
		}

		return false;
	}

	template<typename T, typename U>
	constexpr auto vector_t_good_tuple_v = vector_t_good_tuple_f<T, U>();
}

namespace hades
{
	// fast reciprocal square root
	//computes fast inverse square root
	//eg: 1/sqrt(x)
	//only valid for 32 and 64bit iec559 and ieee754
	template<typename Float,
		typename std::enable_if_t<std::is_floating_point_v<Float>&&
		std::numeric_limits<Float>::is_iec559, int> = 0>
	Float frsqrt(Float) noexcept;

	template<typename T>
	struct vector_t
	{
		using value_type = T;

		constexpr vector_t() noexcept(std::is_nothrow_default_constructible_v<T>) = default;
		//construct from T, or objects holding Ts
		constexpr vector_t(T x, T y) noexcept(std::is_nothrow_constructible_v<T, T>);
		template<typename U, std::enable_if_t<detail::vector_t_good_tuple_v<T, U>, int> = 0>
		explicit constexpr vector_t(const U&) noexcept(std::is_nothrow_constructible_v<T, T>);
		template<typename U, std::enable_if_t<!is_tuple_v<U>, int> = 0>
		explicit constexpr vector_t(const U&) noexcept(std::is_nothrow_constructible_v<T, T>);
		explicit constexpr vector_t(const std::pair<T, T>&) noexcept(std::is_nothrow_constructible_v<T, T>);

		constexpr vector_t(const vector_t&) noexcept(std::is_nothrow_constructible_v<T, T>) = default;

		~vector_t() noexcept(std::is_nothrow_destructible_v<T>) = default;

		constexpr vector_t &operator=(const vector_t&) noexcept(std::is_nothrow_copy_assignable_v<T>) = default;

		T x{}, y{};
	};

	using vector_int = vector_t<int32>;
	using vector_float = vector_t<float>;

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

	//TODO: this should be pol_vector_t
	template<typename T>
	struct rad_vector_t
	{
		T a, m;
	};

	template<typename T>
	vector_t<T> to_vector(rad_vector_t<T> v);

	template<typename T>
	rad_vector_t<T> to_rad_vector(vector_t<T>);

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
		constexpr T x_comp(rad_vector_t<T>) noexcept;

		template<typename T>
		constexpr T y_comp(rad_vector_t<T>) noexcept;

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
	}
}

#include "hades/detail/vector_math.inl"

#endif // !HADES_UTIL_VECTOR_MATH_HPP