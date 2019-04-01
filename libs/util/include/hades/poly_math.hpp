#ifndef HADES_UTIL_POLY_MATH_HPP
#define HADES_UTIL_POLY_MATH_HPP

#include <vector>

#include "hades/vector_math.hpp"

namespace hades
{
	template<typename T>
	struct polygon_t
	{
		vector_t<T> position;
		std::vector<vector_t<T>> vertex;
	};

	namespace polygon
	{
		//#ref: https://www.youtube.com/watch?v=7Ik2vowGcU0

		//moves the position of the poly and translates the vertex
		//the total location of all vertex stays the same,
		//but the position will have moved by the amount passed in
		//move_origin()
	}
}

#include "hades/detail/poly_math.inl"

#endif // !HADES_UTIL_POLY_MATH_HPP