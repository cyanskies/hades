#ifndef HADES_UTIL_VECTOR_MATH_HPP
#define HADES_UTIL_VECTOR_MATH_HPP

#include <optional>
#include <tuple>
#include <type_traits>

#include "hades/math.hpp"
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
	struct basic_vector : public detail::vector_xy_comp<T>,
		// add z component
		public std::conditional_t < 2 < Size, detail::vector_z_comp<T>, detail::vector_z_empty > ,
		// add w component
		public std::conditional_t < 3 < Size, detail::vector_w_comp<T>, detail::vector_w_empty >
	{
		static_assert(Size > 1, "basic_vector doesn't support 1d vectors");
		static_assert(Size < 5, "basic_vector only supports up to vector4");
		using value_type = T;

		[[nodiscard]] constexpr std::size_t size() noexcept;

		constexpr basic_vector& operator+=(const basic_vector& rhs) noexcept;
		constexpr basic_vector& operator-=(const basic_vector& rhs) noexcept;

		//scalar multiplication
		constexpr basic_vector& operator*=(const T rhs) noexcept;
		//scalar division
		constexpr basic_vector& operator/=(const T rhs) noexcept;

		[[nodiscard]] constexpr T& operator[](std::size_t i) noexcept;
		[[nodiscard]] constexpr const T& operator[](std::size_t i) const noexcept;

		template<typename U>
		[[nodiscard]] explicit constexpr operator basic_vector<U, Size>() const noexcept;
	};

	template<typename T>
	struct is_basic_vector : std::false_type {};

	template<typename T, std::size_t N>
	struct is_basic_vector<basic_vector<T, N>> : std::true_type {};

	template<typename T>
	constexpr auto is_basic_vector_v = is_basic_vector<T>::value;

	template<typename T>
	struct basic_vector_size;

	template<typename T, std::size_t N>
	struct basic_vector_size<basic_vector<T, N>> : std::integral_constant<std::size_t, N> {};

	template<typename T>
	constexpr auto basic_vector_size_v = basic_vector_size<T>::value;

	template<typename T>
	using vector2 = basic_vector<T, 2>;
	template<typename T>
	using vector3 = basic_vector<T, 3>;
	template<typename T>
	using vector4 = basic_vector<T, 4>;
	
	using vector2_int = vector2<int32>;
	using vector2_float = vector2<float>;

	static_assert(std::is_trivially_constructible_v<vector4<int32>>);
	static_assert(std::is_trivially_assignable_v<basic_vector<int32>, basic_vector<int32>>);
	static_assert(std::is_trivially_copyable_v<vector4<int32>>);
	static_assert(std::is_trivial_v<vector4<int32>>);

	template<std::floating_point Float, std::size_t N>
	[[nodiscard]] constexpr basic_vector<Float, N> lerp(basic_vector<Float, N> a, basic_vector<Float, N> b, Float t) noexcept
	{
		using std::lerp;
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
	[[nodiscard]] inline constexpr bool float_near_equal(basic_vector<Float, N> a, basic_vector<Float, N> b, int32 units_after_decimal = 2) noexcept
	{
		auto ret = float_near_equal(a.x, b.x, units_after_decimal) && float_near_equal(a.y, b.y, units_after_decimal);
		if constexpr (N > 2)
			ret = ret && float_near_equal(a.z, b.z, units_after_decimal);
		if constexpr (N > 3)
			ret = ret && float_near_equal(a.w, b.w, units_after_decimal);
		return ret;
	}

	template<typename Float, std::size_t N, std::enable_if_t<std::is_floating_point_v<Float>, int> = 0>
	[[nodiscard]] bool float_rounded_equal(basic_vector<Float, N> a, basic_vector<Float, N> b, detail::round_nearest_t = round_nearest_tag) noexcept
	{
		auto ret = float_rounded_equal(a.x, b.x) && float_rounded_equal(a.y, b.y);
		if constexpr (N > 2)
			ret = ret && float_rounded_equal(a.z, b.z);
		if constexpr (N > 3)
			ret = ret && float_rounded_equal(a.w, b.w);
		return ret;
	}

	template<typename Float, std::size_t N, std::enable_if_t<std::is_floating_point_v<Float>, int> = 0>
	[[nodiscard]] bool float_rounded_equal(basic_vector<Float, N> a, basic_vector<Float, N> b, detail::round_down_t) noexcept
	{
		auto ret = float_rounded_equal(a.x, b.x, round_down_tag) && float_rounded_equal(a.y, b.y, round_down_tag);
		if constexpr (N > 2)
			ret = ret && float_rounded_equal(a.z, b.z, round_down_tag);
		if constexpr (N > 3)
			ret = ret && float_rounded_equal(a.w, b.w, round_down_tag);
		return ret;
	}

	template<typename Float, std::size_t N, std::enable_if_t<std::is_floating_point_v<Float>, int> = 0>
	[[nodiscard]] bool float_rounded_equal(basic_vector<Float, N> a, basic_vector<Float, N> b, detail::round_up_t) noexcept
	{
		auto ret = float_rounded_equal(a.x, b.x, round_up_tag) && float_rounded_equal(a.y, b.y, round_up_tag);
		if constexpr (N > 2)
			ret = ret && float_rounded_equal(a.z, b.z, round_up_tag);
		if constexpr (N > 3)
			ret = ret && float_rounded_equal(a.w, b.w, round_up_tag);
		return ret;
	}

	template<typename Float, std::size_t N, std::enable_if_t<std::is_floating_point_v<Float>, int> = 0>
	[[nodiscard]] bool float_rounded_equal(basic_vector<Float, N> a, basic_vector<Float, N> b, detail::round_towards_zero_t) noexcept
	{
		auto ret = float_rounded_equal(a.x, b.x, round_towards_zero_tag) && float_rounded_equal(a.y, b.y, round_towards_zero_tag);
		if constexpr (N > 2)
			ret = ret && float_rounded_equal(a.z, b.z, round_towards_zero_tag);
		if constexpr (N > 3)
			ret = ret && float_rounded_equal(a.w, b.w, round_towards_zero_tag);
		return ret;
	}

	template<typename T, std::size_t N>
	[[nodiscard]] constexpr bool operator==(const basic_vector<T, N> &lhs, const basic_vector<T, N> &rhs) noexcept;

	template<typename T, std::size_t N>
	[[nodiscard]] constexpr bool operator!=(const basic_vector<T, N> &lhs, const basic_vector<T, N> &rhs) noexcept;

	template<typename T>
	[[nodiscard]] constexpr vector2<T> operator+(const vector2<T> &lhs, const vector2<T> &rhs) noexcept;

	template<typename T, std::size_t N>
	[[nodiscard]] constexpr basic_vector<T, N> operator-(const basic_vector<T, N>&lhs, const basic_vector<T, N>&rhs) noexcept;

	template<typename T, typename U, std::enable_if_t<std::is_convertible_v<U, T>, int> = 0>
	[[nodiscard]] constexpr vector2<T> operator*(const vector2<T> &lhs, U rhs) noexcept;

	template<typename T>
	[[nodiscard]] constexpr vector2<T> operator/(const vector2<T> &lhs, T rhs) noexcept;

	namespace detail
	{
		template<typename T>
		struct vector_theta_magnitude_component
		{
			T theta, magnitude;
		};

		template<typename T>
		struct vector_phi_component
		{
			T phi;
		};

		struct vector_phi_component_empty {};
	}

	template<typename T, std::size_t Size = 2>
	struct basic_pol_vector : public detail::vector_theta_magnitude_component<T>,
		public std::conditional_t<3 < Size, detail::vector_phi_component<T>, detail::vector_phi_component_empty>
	{
		static_assert(Size > 1, "basic_pol_vector doesn't support 1d vectors");
		static_assert(Size < 4, "basic_pol_vector only supports up to vector3");
		using value_type = T;

		[[nodiscard]] constexpr std::size_t size() noexcept;
	};

	template<typename T>
	using pol_vector2_t = basic_pol_vector<T>;

	template<typename T, std::size_t Size>
	[[nodiscard]] constexpr basic_vector<T, Size> to_vector(basic_pol_vector<T, Size> v) noexcept;

	template<typename T, std::size_t Size>
	[[nodiscard]] constexpr basic_pol_vector<T, Size> to_pol_vector(basic_vector<T, Size>) noexcept;

	namespace vector
	{
		//returns the length of the vector
		// NOTE: cannot be constexpr until cpp26(std::hypot)
		template<typename T, std::size_t Size>
		[[nodiscard]] auto magnitude(basic_vector<T, Size>) noexcept;

		template<typename T, std::size_t Size>
		[[nodiscard]] constexpr T magnitude_squared(basic_vector<T, Size>) noexcept;

		//returns the angle_theta of the vector compared to the vector [1, 0]
		// returns in degrees
		// TODO: return in radians
		template<typename T, std::size_t Size>
		[[nodiscard]] constexpr auto angle_theta(basic_vector<T, Size>) noexcept;
		template<typename T, std::size_t Size>
		[[nodiscard]] constexpr auto angle_theta_degrees(basic_vector<T, Size> v) noexcept
		{
			return to_degrees(angle_theta(v));
		}

		// NOTE: unlike above, returns angle in radians
		template<typename T, std::size_t Size>
			requires (Size > 2)
		[[nodiscard]] constexpr auto angle_phi(basic_vector<T, Size>, std::optional<T> magnitude = {}) noexcept;

		// returns the angle_theta between the two vectors
		// returns in radians
		template<typename T>
		[[nodiscard]] auto angle(vector2<T>, vector2<T>) noexcept;

		template<typename T, std::size_t Size>
		[[nodiscard]] constexpr T x_comp(basic_pol_vector<T, Size>) noexcept;

		template<typename T, std::size_t Size>
		[[nodiscard]] constexpr T y_comp(basic_pol_vector<T, Size>) noexcept;

		template<typename T, std::size_t Size>
			requires (Size > 2)
		[[nodiscard]] constexpr T z_comp(basic_pol_vector<T, Size>) noexcept;

		//changes the length of a vector to match the provided length
		template<typename T, std::size_t Size>
		[[nodiscard]] constexpr basic_vector<T, Size> resize(basic_vector<T, Size>, T length) noexcept;

		template<typename T, std::size_t Size>
		[[nodiscard]] constexpr basic_vector<T, Size> unit(basic_vector<T, Size>) noexcept;

		template<typename T>
		auto distance(vector2<T>, vector2<T>) noexcept;

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

		// NOTE: cross is only meaningly defined for 3d and 7d vectors(we aren't going to support 7d vectors lol)
		template<typename T>
		constexpr basic_vector<T, 3> cross(basic_vector<T, 3> a, basic_vector<T, 3> b) noexcept;

		template <typename T>
		constexpr vector2<T> project(vector2<T> vector, vector2<T> axis) noexcept;

		template <typename T>
		constexpr vector2<T> reflect(vector2<T> vector, vector2<T> normal) noexcept;
	}

	template<typename T, std::size_t N>
	string vector_to_string(basic_vector<T, N>);
}

#include "hades/detail/vector_math.inl"

#endif // !HADES_UTIL_VECTOR_MATH_HPP