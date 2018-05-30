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
			{ c[bottom_left] - c[top_left] }, //left
			{ c[bottom_right] - c[top_right] }, //right
			{ c[top_right] - c[top_left] }, //top
			{ c[bottom_right] - c[bottom_left] }  //bottom
		};
	}

	template<typename T>
	vector_t<T> intersect(line_t<T> first, line_t<T> second)
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

		assert(denominator != 0); // 0 means the lines are parallel.

		const auto px_numerator = numerator_start * (x3 - x4) - (x1 - x2) * numerator_end;
		const auto px = px_numerator / denominator;

		const auto py_numerator = numerator_start * (y3 - y4) - (y1 - y2) * numerator_end
		const auto py = py_numerator / denominator;

		return { px, py };
	}
}