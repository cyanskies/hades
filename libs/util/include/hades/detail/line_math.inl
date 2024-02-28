#include "hades/line_math.hpp"

#include <array>

#include "hades/math.hpp"

namespace hades::line
{
	template<typename T>
	std::array<line_t<T>, 4> from_rect(rect_t<T> r)
	{
		const auto c = corners(r);

		return {
			line_t<T>{ c[static_cast<std::size_t>(rect_corners::bottom_left)], c[static_cast<std::size_t>(rect_corners::top_left)] }, //left
			line_t<T>{ c[static_cast<std::size_t>(rect_corners::bottom_right)], c[static_cast<std::size_t>(rect_corners::top_right)] }, //right
			line_t<T>{ c[static_cast<std::size_t>(rect_corners::top_right)], c[static_cast<std::size_t>(rect_corners::top_left)] }, //top
			line_t<T>{ c[static_cast<std::size_t>(rect_corners::bottom_right)], c[static_cast<std::size_t>(rect_corners::bottom_left)] }  //bottom
		};
	}

	template<typename T>
	constexpr equation<T> to_equation(const line_t<T> l) noexcept
	{
		return { 
			l.s.y - l.e.y,
			l.e.x - l.s.x,
			(l.s.x - l.e.x) * l.s.y + (l.e.y - l.s.y) * l.s.x
		};
	}

	template<typename T>
	constexpr bool above(const point_t<T> p, const line_t<T> l) noexcept
	{
		const auto s = (l.e.y - l.s.y) * p.x + (l.s.x - l.e.x) * p.y + (l.e.x * l.s.y - l.s.x * l.e.y);
		return s >= 0;
	}

	template<typename T>
	constexpr T distance(const point_t<T> p, const line_t<T> l) noexcept
	{
		const auto e = to_equation(l);
		return std::abs(e.a * p.x + e.b * p.y + e.c) / std::hypot(e.a, e.b);
	}

	template<typename T>
	std::optional<vector2<T>> intersect(line_t<T> first, line_t<T> second)
	{
		//based on the determinants algorithm
		const auto x1 = first.s.x,
			x2 = first.e.x,
			x3 = second.s.x,
			x4 = second.e.x;

		const auto y1 = first.s.y,
			y2 = first.e.y,
			y3 = second.s.y,
			y4 = second.e.y;

		const auto numerator_start = x1 * y2 - y1 * x2;
		const auto numerator_end = x3 * y4 - y3 * x4;
		const auto denominator = (x1 - x2)*(y3 - y4) - (y1 - y2)*(x3 - x4);

		if (denominator == 0) // 0 means the lines are parallel.
			return std::nullopt;

		const auto px_numerator = numerator_start * (x3 - x4) - (x1 - x2) * numerator_end;
		const auto px = px_numerator / denominator;

		const auto py_numerator = numerator_start * (y3 - y4) - (y1 - y2) * numerator_end;
		const auto py = py_numerator / denominator;

		return vector2<T>{ px, py };
	}
}