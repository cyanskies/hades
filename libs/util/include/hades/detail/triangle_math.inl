#include "hades/triangle_math.hpp"

namespace hades
{
	[[nodiscard]]
	constexpr barycentric_point to_barycentric(const vector2_float p, const basic_triangle<float> tri) noexcept
	{
		const auto& p1 = tri.p1;
		const auto& p2 = tri.p2;
		const auto& p3 = tri.p3;
		const auto t = (p2.y - p3.y) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.y - p3.y);
		const auto u = ((p2.y - p3.y) * (p.x - p3.x) + (p3.x - p2.x) * (p.y - p3.y)) / t;
		const auto v = ((p3.y - p1.y) * (p.x - p3.x) + (p1.x - p3.x) * (p.y - p3.y)) / t;
		const auto w = 1.f - u - v;
		return { u, v, w };
	}

	[[nodiscard]]
	constexpr vector2_float from_barycentric(const barycentric_point b, const basic_triangle<float> t) noexcept
	{
		const auto& p1 = t.p1;
		const auto& p2 = t.p2;
		const auto& p3 = t.p3;

		const auto& [u, v, w] = b;
		return { u * p1.x + v * p2.x + w * p3.x, u * p1.y + v * p2.y + w * p3.y };
	}

	template<typename T>
	[[nodiscard]]
	constexpr bool is_within(const basic_vector<T, 2> p, const basic_triangle<T> tri) noexcept
	{
		// Based on PointInTriangle from: https://stackoverflow.com/a/20861130
		const auto& p0 = tri.p1;
		const auto& p1 = tri.p2;
		const auto& p2 = tri.p3;

		const auto s = (p0.x - p2.x) * (p.y - p2.y) - (p0.y - p2.y) * (p.x - p2.x);
		const auto t = (p1.x - p0.x) * (p.y - p0.y) - (p1.y - p0.y) * (p.x - p0.x);

		if ((s < 0) != (t < 0) && s != 0 && t != 0)
			return false;

		const auto d = (p2.x - p1.x) * (p.y - p1.y) - (p2.y - p1.y) * (p.x - p1.x);
		return d == 0 || (d < 0) == (s + t <= 0);
	}

	template<typename T>
	[[nodiscard]]
	constexpr bool is_within(basic_vector<T, 2> p, std::array<basic_vector<T, 2>, 3> tri) noexcept
	{
		return is_within(p, basic_triangle<T>{ tri[0], tri[1], tri[2] });
	}
}