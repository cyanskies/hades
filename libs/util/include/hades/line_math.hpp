#ifndef HADES_UTIL_LINE_MATH_HPP
#define HADES_UTIL_LINE_MATH_HPP

#include <array>
#include <optional>
#include <tuple>

#include "hades/rectangle_math.hpp"
#include "hades/vector_math.hpp"

namespace hades
{
	template<typename T>
	struct line_t
	{
		vector2<T> s, e;
	};

	namespace line
	{
		template<typename T>
		std::array<line_t<T>, 4> from_rect(rect_t<T>);

		//returns the intersect point for the two lines
		//returns nothing if the lines are parellel
		template<typename T>
		std::optional<vector2<T>> intersect(line_t<T> first, line_t<T> second);
	}
}

#include "hades/detail/line_math.inl"

#endif // !HADES_UTIL_LINE_MATH_HPP