#include "hades/vector_math.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <tuple>
#include <type_traits>

#include "hades/utility.hpp"

namespace hades
{
	template<typename T>
	template<typename U>
	inline constexpr vector_t<T>::operator vector_t<U>() const noexcept
	{
		if constexpr (std::is_integral_v<T> && std::is_integral_v<U>)
		{
			return {
				integer_cast<U>(x),
				integer_cast<U>(y)
			};
		}
		else if constexpr(std::is_integral_v<U>)
		{
			//convert float to integral
			return {
				integral_cast<U>(x),
				integral_cast<U>(y)
			};
		}
		else if constexpr (std::is_floating_point_v<U>)
		{
			//convert to float(this protects against out of range casts from double -> float)
			return {
				float_cast<U>(x),
				float_cast<U>(y)
			};
		}
		
		/*return {
				static_cast<U>(x),
				static_cast<U>(y)
		};*/
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

	template<typename T, typename U, std::enable_if_t<std::is_convertible_v<U, T>, int>>
	constexpr vector_t<T> operator*(const vector_t<T> &lhs, U rhs) noexcept
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
		T magnitude(vector_t<T> v)
		{
			const auto mag = std::hypot(v.x, v.y);
			//hypot returns either float or double
			if constexpr (std::is_integral_v<T>)
				return integral_cast<T>(mag);
			else
				return float_cast<T>(mag);
		}

		template<typename T>
		constexpr T magnitude_squared(vector_t<T> v) noexcept
		{
			return v.x*v.x + v.y*v.y;
		}

		template<typename T>
		T angle(vector_t<T> v) noexcept
		{
			auto ret = T{};
			//NOTE: y is inverted because opengl
			if constexpr (std::is_floating_point_v<T>)
                ret = std::atan2(v.y * -1.f, v.x) * 180 / std::numbers::pi_v<T>;
			else
                ret = static_cast<T>(std::atan2(static_cast<float>(v.y) * -1.f, static_cast<float>(v.x)) * 180 / std::numbers::pi_v<float>);

			//atan2 reurns in the range {180, -180}
			// convert to {0, 360}
			if (ret < 0)
				ret += 360;

			return ret;
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
		vector_t<T> resize(vector_t<T> v, T length) noexcept
		{
			return unit(v) * length;
		}

		template<typename T>
		vector_t<T> unit(vector_t<T> v) noexcept
		{
			const auto inverse_root = 1 / std::sqrt(magnitude_squared(v));
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
