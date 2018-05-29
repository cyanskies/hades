#ifndef HADES_COLLISION_HPP
#define HADES_COLLISION_HPP

#include <tuple>
#include <vector>

#include "hades/types.hpp"
#include "hades/vector_math.hpp"

namespace hades
{
	template<typename T>
	using point_t = vector_t<T>;

	template<typename T>
	struct rect_t
	{
		T x, y, width, height;
	};

	template<typename T>
	struct circle_t
	{
		T x, y, r;
	};

	template<typename T>
	struct multipoint_t
	{
		std::vector<point_t<T>> points;
	};

	//returns true if the objects are intersecting
	template<typename U, typename V>
	bool collision_test(U first, V second);

	enum class direction {
		left,
		right,
		top,
		bottom
	};

	//returns a tuple of collided, max move without intersection
	//prev is required to not be intersecting
	template<typename T, template<typename> typename U, template<typename> typename V>
	std::tuple<bool, vector_t<T>> collision_test(U<T> prev, U<T> current, V<T> other);

	//returns the direction the collision occured from
	template<typename T, template<typename> typename U>
	direction collision_direction(U<T> prev, U<T> current, rect_t<T> other);

	//generates a bounding box that at least covers the whole of the collision primative.
	template<typename T, template<typename U<T>>>
	rect_t<T> bounding_box(U<T> object);
}

#include "hades/detail/collision.inl"

#endif //HADES_COLLISION_HPP
