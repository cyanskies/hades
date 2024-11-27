#ifndef HADES_TRIANGLE_MATH_HPP
#define HADES_TRIANGLE_MATH_HPP

#include "hades/vector_math.hpp"

namespace hades
{
	template<typename T>
	struct basic_triangle
	{
		basic_vector<T, 2> p1, p2, p3;
	};

	struct barycentric_point
	{
		float u, v, w;
	};

	constexpr bool operator==(const barycentric_point& lhs, const barycentric_point& rhs) noexcept
	{
		return std::tie(lhs.u, lhs.v, lhs.w) == std::tie(rhs.u, rhs.v, rhs.w);
	}

	[[nodiscard]]
	constexpr barycentric_point to_barycentric(vector2_float, basic_triangle<float>) noexcept;

	constexpr barycentric_point to_barycentric(const vector2_float p, const std::array<vector2_float, 3>& tri) noexcept
	{
		return to_barycentric(p, basic_triangle<float>{ tri[0], tri[1], tri[2] });
	}

	[[nodiscard]]
	constexpr vector2_float from_barycentric(barycentric_point, basic_triangle<float>) noexcept;

	constexpr vector2_float from_barycentric(const barycentric_point b, const std::array<vector2_float, 3>& tri) noexcept
	{
		return from_barycentric(b, basic_triangle<float>{ tri[0], tri[1], tri[2] });
	}

	template<typename T>
	[[nodiscard]]
	constexpr bool is_within(basic_vector<T, 2>, basic_triangle<T>) noexcept;

	template<typename T>
	[[nodiscard]]
	constexpr bool is_within(basic_vector<T, 2>, std::array<basic_vector<T, 2>, 3>) noexcept;
}

#include "hades/detail/triangle_math.inl"

#endif // !HADES_TRIANGLE_MATH_HPP