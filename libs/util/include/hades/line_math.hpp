#ifndef HADES_UTIL_LINE_MATH_HPP
#define HADES_UTIL_LINE_MATH_HPP

#include <array>
#include <tuple>

#include "hades/vector_math.hpp"

namespace hades
{
	template<typename T>
	struct line_t
	{
		vector_t<T> s, e;
	};

	namespace line
	{
		template<typename T>
		std::array<line_t<T>, 4> from_rect(rect_t<T>);

		template<typename T>
		vector_t<T> intersect(line_t<T> first, line_t<T> second);
	}
}

#include "hades/detail/line_math.inl"

#endif // !HADES_UTIL_LINE_MATH_HPP