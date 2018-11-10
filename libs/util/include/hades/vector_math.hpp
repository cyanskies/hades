#ifndef HADES_UTIL_VECTOR_MATH_HPP
#define HADES_UTIL_VECTOR_MATH_HPP

#include <optional>
#include <tuple>

#include "hades/types.hpp"

namespace hades
{
	template<typename T>
	struct vector_t
	{
		using value_type = T;

		constexpr vector_t() noexcept = default;
		constexpr vector_t(T x, T y) noexcept : x(x), y(y) {}

		~vector_t() noexcept = default;
	
		template<typename U>
		constexpr vector_t(const vector_t<U> &other) noexcept : x(static_cast<T>(other.x)), y(static_cast<T>(other.y))
		{}

		template<typename U>
		constexpr vector_t(vector_t<U> &&other) noexcept : x(static_cast<T>(other.x)), y(static_cast<T>(other.y))
		{}

		template<typename U>
		constexpr vector_t &operator=(const vector_t<U> &other) noexcept
		{
			x = static_cast<T>(other.x);
			y = static_cast<T>(other.y);
			return *this;
		}

		template<typename U>
		constexpr vector_t &operator=(vector_t<U> &&other) noexcept
		{
			x = static_cast<T>(other.x);
			y = static_cast<T>(other.y);
			return *this;
		}

		T x{}, y{};
	};

	using vector_int = vector_t<int32>;
	using vector_float = vector_t<float>;

	template<typename T>
	constexpr bool operator==(const vector_t<T> &lhs, const vector_t<T> &rhs);

	template<typename T>
	constexpr bool operator!=(const vector_t<T> &lhs, const vector_t<T> &rhs);

	template<typename T>
	constexpr vector_t<T> operator+(const vector_t<T> &lhs, const vector_t<T> &rhs);

	template<typename T>
	constexpr vector_t<T> operator-(const vector_t<T> &lhs, const vector_t<T> &rhs);

	template<typename T>
	constexpr vector_t<T> operator*(const vector_t<T> &lhs, T rhs);

	template<typename T>
	constexpr vector_t<T> operator/(const vector_t<T> &lhs, T rhs);

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
		constexpr T magnitude(vector_t<T>);

		template<typename T>
		constexpr T magnitude_squared(vector_t<T> v);

		//returns the angle of the vector
		template<typename T>
		constexpr T angle(vector_t<T>);

		template<typename T>
		T x_comp(rad_vector_t<T>);

		template<typename T>
		T y_comp(rad_vector_t<T>);

		//changes the length of a vector to match the provided length
		template<typename T>
		constexpr vector_t<T> resize(vector_t<T>, T length);

		template<typename T>
		constexpr vector_t<T> unit(vector_t<T>);

		template<typename T>
		constexpr T distance(vector_t<T>, vector_t<T>);

		template<typename T>
		constexpr vector_t<T> reverse(vector_t<T>);

		//returns a vector that points 90 degrees of the origional vector
		template<typename T>
		constexpr vector_t<T> perpendicular(vector_t<T>);

		//returns a vector that points 280 degrees of the origional vector
		template<typename T>
		constexpr vector_t<T> perpendicular_reverse(vector_t<T>);

		template<typename T>
		constexpr vector_t<T> clamp(vector_t<T> value, vector_t<T> min, vector_t<T> max);

		template<typename T>
		constexpr T dot(vector_t<T> a, vector_t<T> b);

		template <typename T>
		constexpr vector_t<T> project(vector_t<T> vector, vector_t<T> axis);
	}
}

#include "hades/detail/vector_math.inl"

#endif // !HADES_UTIL_VECTOR_MATH_HPP