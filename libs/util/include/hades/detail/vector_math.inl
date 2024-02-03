#include "hades/vector_math.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <numbers>
#include <numeric>
#include <tuple>
#include <type_traits>

#include "hades/tuple.hpp"
#include "hades/utility.hpp"

namespace hades
{
	template<typename T, std::size_t Size>
	constexpr std::size_t basic_vector<T, Size>::size() noexcept
	{
		return Size;
	}

	template<typename T, std::size_t Size>
	constexpr basic_vector<T, Size>& basic_vector<T, Size>::operator+=(const basic_vector& rhs) noexcept
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
	constexpr basic_vector<T, Size>& basic_vector<T, Size>::operator-=(const basic_vector& rhs) noexcept
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
	constexpr basic_vector<T, Size>& basic_vector<T, Size>::operator*=(const T rhs) noexcept
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
	constexpr basic_vector<T, Size>& basic_vector<T, Size>::operator/=(const T rhs) noexcept
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
	template<typename U>
	inline constexpr basic_vector<T, Size>::operator basic_vector<U, Size>() const noexcept
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
			return basic_vector<U>{
				cast_func(this->x),
				cast_func(this->y)
			};
		}
		else if constexpr (Size > 2)
		{
			return basic_vector<U, Size>{
				cast_func(this->x),
				cast_func(this->y),
				cast_func(this->z)
			};
		}
		else if constexpr (Size > 3)
		{
			return basic_vector<U, Size>{
				cast_func(this->x),
				cast_func(this->y),
				cast_func(this->z),
				cast_func(this->w)
			};
		}
	}

	template<typename T, std::size_t N>
	constexpr bool operator==(const basic_vector<T, N> &lhs, const basic_vector<T, N> &rhs) noexcept
	{
		auto ret = lhs.x == rhs.x && lhs.y == rhs.y;
		if constexpr (N > 2)
			ret = ret && lhs.z == rhs.z;
		if constexpr (N > 3)
			ret = ret && lhs.w == rhs.w;
		return ret;
	}

	template<typename T, std::size_t N>
	constexpr bool operator!=(const basic_vector<T, N> &lhs, const basic_vector<T, N> &rhs) noexcept
	{
		return !(lhs == rhs);
	}

	template<typename T>
	constexpr vector2<T> operator+(const vector2<T> &lhs, const vector2<T> &rhs) noexcept
	{
		return { lhs.x + rhs.x, lhs.y + rhs.y };
	}

	template<typename T, std::size_t N>
	constexpr basic_vector<T, N> operator-(const basic_vector<T, N>& lhs, const basic_vector<T, N>& rhs) noexcept
	{
		auto ret = lhs;
		return ret -= rhs;
	}

	template<typename T, std::size_t N, typename U>
		requires std::convertible_to<U, T>
	[[nodiscard]] constexpr basic_vector<T, N> operator*(const basic_vector<T, N>& lhs, const U rhs) noexcept
	{
		const auto r = T{ rhs };
		auto out = basic_vector<T, N>{ lhs.x * r, lhs.y * r };
		if constexpr (N > 2)
			out.z *= r;
		if constexpr (N > 3)
			out.w *= r;
		return out;
	}

	template<typename T>
	constexpr vector2<T> operator/(const vector2<T> &lhs, T rhs) noexcept
	{
		return { lhs.x / rhs, lhs.y / rhs };
	}

	template<typename T, std::size_t Size>
	constexpr std::size_t basic_pol_vector<T, Size>::size() noexcept
	{
		return Size;
	}

	template<typename T, std::size_t Size>
	constexpr basic_vector<T, Size> to_vector(basic_pol_vector<T, Size> v) noexcept
	{
		auto out = basic_vector<T, Size>{ vector::x_comp(v), vector::y_comp(v) };
		if constexpr (Size > 2)
			out.z = vector::z_comp(v);
		return out;
	}

	template<typename T, std::size_t Size>
	constexpr basic_pol_vector<T, Size> to_pol_vector(basic_vector<T, Size> v) noexcept
	{
		const auto mag = vector::magnitude(v);
		auto out = basic_pol_vector<T, Size>{ vector::angle_theta_degrees(v), mag };
		if constexpr (Size > 2)
			out.phi = vector::angle_phi(v, mag);
		return out;
	}

	template<typename T, std::size_t N>
	string vector_to_string(basic_vector<T, N> v)
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
		template<typename T, std::size_t Size>
		auto magnitude(const basic_vector<T, Size> v) noexcept
		{
			if constexpr (Size == 2)
				return std::hypot(v.x, v.y);
			else if constexpr (Size < 4)
				return std::hypot(v.x, v.y, v.z);
			else
			{
				return std::sqrt(magnitude_squared(v));
			}
		}

		template<typename T, std::size_t Size>
		constexpr T magnitude_squared(const basic_vector<T, Size> v) noexcept
		{
			static_assert(Size < 5);
			auto out = v.x * v.x + v.y * v.y;
			if constexpr (Size > 2)
				out += v.z * v.z;
			if constexpr (Size > 3)
				out += v.w * v.w;

			return out;
		}

		template<typename T, std::size_t Size>
		constexpr auto angle_theta(const basic_vector<T, Size> v) noexcept
		{
			static_assert(Size < 4);
			if constexpr (std::is_floating_point_v<T>)
				return to_degrees(std::atan2(v.y, v.x));
			else
				return to_degrees(std::atan2(static_cast<float>(v.y),
					static_cast<float>(v.x)));
		}

		template<typename T, std::size_t Size>
			requires (Size > 2)
		constexpr auto angle_phi(const basic_vector<T, Size> v, std::optional<T> mag) noexcept
		{
			static_assert(Size < 4);

			if constexpr (std::is_floating_point_v<T>)
			{
				constexpr auto z_over_mag = v.z / mag.value_or(magnitude(v));
				return std::acos(z_over_mag);
			}
			else
			{
				const auto float_vec = static_cast<basic_vector<float, Size>>(v);
				return angle_phi(float_vec, mag);
			}
		}

		template<typename T>
		auto angle(vector2<T> a, vector2<T> b) noexcept
		{
			const auto d = dot(a, b);
			const auto determinant = a.x * b.y - a.y * b.x;
			return std::atan2(determinant, d);
		}

		template<typename T, std::size_t Size>
		constexpr T x_comp(const basic_pol_vector<T, Size> v) noexcept
		{
			auto out = v.magnitude * std::cos(v.theta);

			if constexpr (Size > 2)
			{
				out *= std::sin(v.phi);
			}
			
			static_assert(Size < 4);
			return out;
		}

		template<typename T, std::size_t Size>
		constexpr T y_comp(const basic_pol_vector<T, Size> v) noexcept
		{
			auto out = v.magnitude * std::sin(v.theta);

			if constexpr (Size > 2)
			{
				out *= std::sin(v.phi);
			}

			static_assert(Size < 4);
			return out;
		}

		template<typename T, std::size_t Size>
			requires (Size > 2)
		constexpr T z_comp(const basic_pol_vector<T, Size> v) noexcept
		{
			static_assert(Size < 4);
			return std::cos(v.phi);
		}

		template<typename T, std::size_t Size>
		constexpr basic_vector<T, Size> resize(const basic_vector<T, Size> v, const T length) noexcept
		{
			return unit(v) * length;
		}

		template<typename T, std::size_t Size>
		constexpr basic_vector<T, Size> unit(const basic_vector<T, Size> v) noexcept
		{
			const auto inverse_root = 1 / magnitude(v);
			return v * inverse_root;
		}

		template<typename T>
		auto distance(vector2<T> a, vector2<T> b) noexcept
		{
			const auto ab = b - a;
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

		template<typename T, std::size_t Size>
		constexpr T dot(const basic_vector<T, Size> a, const basic_vector<T, Size> b) noexcept
		{
			auto a_arr = std::array<T, Size>{ a.x, a.y };
			auto b_arr = std::array<T, Size>{ b.x, b.y };
			if constexpr (Size > 2)
			{
				a_arr[2] = a.z;
				b_arr[2] = b.z;
			}
			if constexpr( Size > 3)
			{
				a_arr[3] = a.w;
				b_arr[3] = b.w;
			}

			return std::inner_product(begin(a_arr), end(a_arr), begin(b_arr), T{});
		}

		template<typename T>
		constexpr basic_vector<T, 3> cross(const basic_vector<T, 3> a, const basic_vector<T, 3> b) noexcept
		{
			//Nx = UyVz - UzVy
			//Ny = UzVx - UxVz
			//Nz = UxVy - UyVx
			return basic_vector<T, 3>{ 
				a.y * b.z - a.z * b.y,
				a.z * b.x - a.x * b.z,
				a.x * b.y - a.y * b.x
			};
		}

		template <typename T>
		constexpr vector2<T> project(vector2<T> vector, vector2<T> axis) noexcept
		{
			assert(axis != vector2<T>());
			return axis * dot(vector, axis) * (1 / magnitude_squared(axis));
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
