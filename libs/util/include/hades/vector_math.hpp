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
	namespace detail
	{
		// we inherit to add the members, because msvc no_unique_address is broken in cpp20
		struct vector_z_empty {};
		struct vector_w_empty {};

		template<typename T>
		struct vector_xy_comp
		{
			T x, y;
		};

		template<typename T>
		struct vector_z_comp
		{
			T z;
		};

		template<typename T>
		struct vector_w_comp
		{
			T w;
		};
	}

	template<typename T, std::size_t Size = 2>
	struct vector_t : public detail::vector_xy_comp<T>,
		// add z component
		public std::conditional_t < 2 < Size, detail::vector_z_comp<T>, detail::vector_z_empty > ,
		// add w component
		public std::conditional_t < 3 < Size, detail::vector_w_comp<T>, detail::vector_w_empty >
	{
		static_assert(Size > 1, "vector_t doesn't support 1d vectors");
		static_assert(Size < 5, "vector_t only supports up to vector4");
		using value_type = T;

		constexpr std::size_t size() noexcept;

		constexpr vector_t& operator+=(const vector_t& rhs) noexcept;
		constexpr vector_t& operator-=(const vector_t& rhs) noexcept;

		//scalar multiplication
		constexpr vector_t& operator*=(const T rhs) noexcept;
		//scalar division
		constexpr vector_t& operator/=(const T rhs) noexcept;

		constexpr T& operator[](std::size_t i) noexcept;
		constexpr const T& operator[](std::size_t i) const noexcept;

		template<typename U>
		explicit constexpr operator vector_t<U, Size>() const noexcept;
	};

	template<typename T>
	using vector2 = vector_t<T, 2>;
	template<typename T>
	using vector3 = vector_t<T, 3>;
	template<typename T>
	using vector4 = vector_t<T, 4>;
	
	static_assert(std::is_trivially_constructible_v<vector4<int32>>);
	static_assert(std::is_trivially_assignable_v<vector_t<int32>, vector_t<int32>>);
	static_assert(std::is_trivially_copyable_v<vector4<int32>>);
	static_assert(std::is_trivial_v<vector4<int32>>);

	template<typename T>
	struct lerpable<vector_t<T>> : public lerpable<T> {};

	template<typename Float, std::size_t N>
		requires std::floating_point<Float>
	constexpr vector_t<Float, N> lerp(vector_t<Float, N> a, vector_t<Float, N> b, Float t) noexcept
	{
		if constexpr (N < 3)
		{
			return {
				lerp(a.x, b.x, t),
				lerp(a.y, b.y, t)
			};
		}
		else if constexpr (N == 3)
		{
			return {
				lerp(a.x, b.x, t),
				lerp(a.y, b.y, t),
				lerp(a.z, b.z, t)
			};
		}
		else if constexpr(N > 3)
		{
			return {
				lerp(a.x, b.x, t),
				lerp(a.y, b.y, t),
				lerp(a.z, b.z, t),
				lerp(a.w, b.w, t)
			};
		}
	}

	template<typename Float, std::size_t N, std::enable_if_t<std::is_floating_point_v<Float>, int> = 0>
	inline constexpr bool float_near_equal(vector_t<Float, N> a, vector_t<Float, N> b, int32 units_after_decimal = 2) noexcept
	{
		auto ret = float_near_equal(a.x, b.x, units_after_decimal) && float_near_equal(a.y, b.y, units_after_decimal);
		if constexpr (N > 2)
			ret = ret && float_near_equal(a.z, b.z, units_after_decimal);
		if constexpr (N > 3)
			ret = ret && float_near_equal(a.w, b.w, units_after_decimal);
		return ret;
	}

	template<typename Float, std::size_t N, std::enable_if_t<std::is_floating_point_v<Float>, int> = 0>
	bool float_rounded_equal(vector_t<Float, N> a, vector_t<Float, N> b, detail::round_nearest_t = round_nearest_tag) noexcept
	{
		auto ret = float_rounded_equal(a.x, b.x) && float_rounded_equal(a.y, b.y);
		if constexpr (N > 2)
			ret = ret && float_rounded_equal(a.z, b.z);
		if constexpr (N > 3)
			ret = ret && float_rounded_equal(a.w, b.w);
		return ret;
	}

	template<typename Float, std::size_t N, std::enable_if_t<std::is_floating_point_v<Float>, int> = 0>
	bool float_rounded_equal(vector_t<Float, N> a, vector_t<Float, N> b, detail::round_down_t) noexcept
	{
		auto ret = float_rounded_equal(a.x, b.x, round_down_tag) && float_rounded_equal(a.y, b.y, round_down_tag);
		if constexpr (N > 2)
			ret = ret && float_rounded_equal(a.z, b.z, round_down_tag);
		if constexpr (N > 3)
			ret = ret && float_rounded_equal(a.w, b.w, round_down_tag);
		return ret;
	}

	template<typename Float, std::size_t N, std::enable_if_t<std::is_floating_point_v<Float>, int> = 0>
	bool float_rounded_equal(vector_t<Float, N> a, vector_t<Float, N> b, detail::round_up_t) noexcept
	{
		auto ret = float_rounded_equal(a.x, b.x, round_up_tag) && float_rounded_equal(a.y, b.y, round_up_tag);
		if constexpr (N > 2)
			ret = ret && float_rounded_equal(a.z, b.z, round_up_tag);
		if constexpr (N > 3)
			ret = ret && float_rounded_equal(a.w, b.w, round_up_tag);
		return ret;
	}

	template<typename Float, std::size_t N, std::enable_if_t<std::is_floating_point_v<Float>, int> = 0>
	bool float_rounded_equal(vector_t<Float, N> a, vector_t<Float, N> b, detail::round_towards_zero_t) noexcept
	{
		auto ret = float_rounded_equal(a.x, b.x, round_towards_zero_tag) && float_rounded_equal(a.y, b.y, round_towards_zero_tag);
		if constexpr (N > 2)
			ret = ret && float_rounded_equal(a.z, b.z, round_towards_zero_tag);
		if constexpr (N > 3)
			ret = ret && float_rounded_equal(a.w, b.w, round_towards_zero_tag);
		return ret;
	}

	template<typename T, std::size_t N>
	constexpr bool operator==(const vector_t<T, N> &lhs, const vector_t<T, N> &rhs) noexcept;

	template<typename T, std::size_t N>
	constexpr bool operator!=(const vector_t<T, N> &lhs, const vector_t<T, N> &rhs) noexcept;

	template<typename T>
	constexpr vector2<T> operator+(const vector2<T> &lhs, const vector2<T> &rhs) noexcept;

	template<typename T>
	constexpr vector2<T> operator-(const vector2<T> &lhs, const vector2<T> &rhs) noexcept;

	template<typename T, typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
	constexpr vector2<T> operator*(const vector2<T> &lhs, U rhs) noexcept;

	template<typename T>
	constexpr vector2<T> operator/(const vector2<T> &lhs, T rhs) noexcept;

	// TODO: vector2_int, vector2_float
	using vector_int = vector2<int32>;
	using vector_float = vector2<float>;

	//TODO: polar vector type
	template<typename T>
	struct pol_vector_t
	{
		T a, m; // angle, magnitude
	};

	template<typename T>
	vector2<T> to_vector(pol_vector_t<T> v);

	template<typename T> // TODO: rename to_pol_vector2
	pol_vector_t<T> to_rad_vector(vector2<T>);

	namespace vector
	{
		//returns the length of the vector
		template<typename T>
		T magnitude(vector2<T>);

		template<typename T>
		constexpr T magnitude_squared(vector2<T> v) noexcept;

		//returns the angle of the vector compared to the vector [1, 0]
		// returns in degrees
		// TODO: return in radians
		template<typename T>
		[[nodiscard]] auto angle(vector2<T>) noexcept;

		// returns the angle between the two vectors
		// returns in radians
		template<typename T>
		[[nodiscard]] auto angle(vector2<T>, vector2<T>) noexcept;

		template<typename T>
		T x_comp(pol_vector_t<T>) noexcept;

		template<typename T>
		T y_comp(pol_vector_t<T>) noexcept;

		//changes the length of a vector to match the provided length
		template<typename T>
		vector2<T> resize(vector2<T>, T length) noexcept;

		template<typename T>
		vector2<T> unit(vector2<T>) noexcept;

		template<typename T>
		T distance(vector2<T>, vector2<T>) noexcept;

		template<typename T>
		constexpr vector2<T> reverse(vector2<T>) noexcept;

		template<typename T>
		vector2<T> abs(vector2<T>) noexcept;

		//returns a vector that points 90 degrees of the origional vector
		template<typename T>
		constexpr vector2<T> perpendicular(vector2<T>) noexcept;

		//returns a vector that points 280 degrees of the origional vector
		template<typename T>
		constexpr vector2<T> perpendicular_reverse(vector2<T>) noexcept;

		template<typename T>
		constexpr vector2<T> clamp(vector2<T> value, vector2<T> min, vector2<T> max) noexcept;

		template<typename T>
		constexpr T dot(vector2<T> a, vector2<T> b) noexcept;

		template <typename T>
		constexpr vector2<T> project(vector2<T> vector, vector2<T> axis) noexcept;

		template <typename T>
		constexpr vector2<T> reflect(vector2<T> vector, vector2<T> normal) noexcept;
	}

	template<typename T, std::size_t N>
	string vector_to_string(vector_t<T, N>);
}

#include "hades/detail/vector_math.inl"

#endif // !HADES_UTIL_VECTOR_MATH_HPP