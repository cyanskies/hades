#ifndef HADES_UTIL_VECTOR_MATH_HPP
#define HADES_UTIL_VECTOR_MATH_HPP

#include <optional>
#include <tuple>
#include <type_traits>

#include "hades/string.hpp"
#include "hades/types.hpp"
#include "hades/utility.hpp"

namespace hades
{
	// TODO: rename vector2
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

		constexpr vector_t& operator-=(const vector_t& rhs) noexcept
		{
			x -= rhs.x;
			y -= rhs.y;
			return *this;
		}

		//scalar multiplication
		constexpr vector_t& operator*=(const T rhs) noexcept
		{
			x *= rhs;
			y *= rhs;
			return *this;
		}

		//scalar division
		constexpr vector_t& operator/=(const T rhs) noexcept
		{
			x /= rhs;
			y /= rhs;
			return *this;
		}

		T& operator[](std::size_t i) noexcept
		{
			constexpr auto arr = std::array{ &vector_t::x, &vector_t::y };
			assert(i < size(arr));
			return std::invoke(arr[i], this);
		}

		const T& operator[](std::size_t i) const noexcept
		{
			constexpr auto arr = std::array{ &vector_t::x, &vector_t::y };
			assert(i < size(arr));
			return std::invoke(arr[i], this);
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
	struct lerpable<vector_t<T>> : public lerpable<T> {};

	template<typename Float>
		requires std::floating_point<Float>
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
		return float_near_equal(a.x, b.x, units_after_decimal) && float_near_equal(a.y, b.y, units_after_decimal);
	}

	template<typename Float, template<typename> typename Vector, std::enable_if_t<std::is_floating_point_v<Float>, int> = 0>
	bool float_rounded_equal(Vector<Float> a, Vector<Float> b, detail::round_nearest_t = round_nearest_tag) noexcept
	{
		return float_rounded_equal(a.x, b.x) && float_rounded_equal(a.y, b.y);
	}

	template<typename Float, template<typename> typename Vector, std::enable_if_t<std::is_floating_point_v<Float>, int> = 0>
	bool float_rounded_equal(Vector<Float> a, Vector<Float> b, detail::round_down_t) noexcept
	{
		return float_rounded_equal(a.x, b.x, round_down_tag) && float_rounded_equal(a.y, b.y, round_down_tag);
	}

	template<typename Float, template<typename> typename Vector, std::enable_if_t<std::is_floating_point_v<Float>, int> = 0>
	bool float_rounded_equal(Vector<Float> a, Vector<Float> b, detail::round_up_t) noexcept
	{
		return float_rounded_equal(a.x, b.x, round_up_tag) && float_rounded_equal(a.y, b.y, round_up_tag);
	}

	template<typename Float, template<typename> typename Vector, std::enable_if_t<std::is_floating_point_v<Float>, int> = 0>
	bool float_rounded_equal(Vector<Float> a, Vector<Float> b, detail::round_towards_zero_t) noexcept
	{
		return float_rounded_equal(a.x, b.x, round_towards_zero_tag) && float_rounded_equal(a.y, b.y, round_towards_zero_tag);
	}

	template<typename T>
	constexpr bool operator==(const vector_t<T> &lhs, const vector_t<T> &rhs) noexcept;

	template<typename T>
	constexpr bool operator!=(const vector_t<T> &lhs, const vector_t<T> &rhs) noexcept;

	template<typename T>
	constexpr vector_t<T> operator+(const vector_t<T> &lhs, const vector_t<T> &rhs) noexcept;

	template<typename T>
	constexpr vector_t<T> operator-(const vector_t<T> &lhs, const vector_t<T> &rhs) noexcept;

	template<typename T, typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
	constexpr vector_t<T> operator*(const vector_t<T> &lhs, U rhs) noexcept;

	template<typename T>
	constexpr vector_t<T> operator/(const vector_t<T> &lhs, T rhs) noexcept;

	using vector_int = vector_t<int32>;
	using vector_float = vector_t<float>;

	//TODO: polar vector type
	template<typename T>
	struct pol_vector_t
	{
		T a, m; // angle, magnitude
	};

	template<typename T>
	vector_t<T> to_vector(pol_vector_t<T> v);

	template<typename T> // TODO: rename to_pol_vector2
	pol_vector_t<T> to_rad_vector(vector_t<T>);

	namespace vector
	{
		//returns the length of the vector
		template<typename T>
		T magnitude(vector_t<T>);

		template<typename T>
		constexpr T magnitude_squared(vector_t<T> v) noexcept;

		//returns the angle of the vector compared to the vector [1, 0]
		// returns in degrees
		// TODO: return in radians
		template<typename T>
		[[nodiscard]] auto angle(vector_t<T>) noexcept;

		// returns the angle between the two vectors
		// returns in radians
		template<typename T>
		[[nodiscard]] auto angle(vector_t<T>, vector_t<T>) noexcept;

		template<typename T>
		T x_comp(pol_vector_t<T>) noexcept;

		template<typename T>
		T y_comp(pol_vector_t<T>) noexcept;

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

	template<typename T>
	string vector_to_string(vector_t<T>);
}

#include "hades/detail/vector_math.inl"

#endif // !HADES_UTIL_VECTOR_MATH_HPP