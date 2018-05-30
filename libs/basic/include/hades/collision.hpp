#ifndef HADES_COLLISION_HPP
#define HADES_COLLISION_HPP

#include <tuple>
#include <vector>

#include "hades/types.hpp"
#include "hades/vector_math.hpp"

//collision detection functions for the following types
//rect_t
//point_t
//circle_t
//multipoint_t

//algorithms work best with floating point types.

namespace hades
{
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

	//collision functions are designed to work with
	//point_t //NOTE: point_t never collides with itself or multipoint
	//rect_t
	//circle_t
	//multipoint_t //NOTE: same as for point_t

	//returns true if the objects are intersecting
	template<typename U, typename V>
	bool collision_test(U first, V second);

	enum class direction {
		left,
		right,
		top,
		bottom
	};

	//returns the move needed to bring prev as close as possible to other without colliding.
	//NOTE: prev must not be colliding with other.
	template<typename T, template<typename> typename U, template<typename> typename V>
	std::tuple<bool, vector_t<T>> collision_test(U<T> prev, U<T> current, V<T> other);

	//returns the direction the collision occured from
	template<typename T, template<typename> typename U>
	direction collision_direction(U<T> prev, U<T> current, rect_t<T> other);

	//generates a bounding box that at least covers the whole of the collision primative.
	template<typename T, template<typename> typename U>
	rect_t<T> bounding_box(U<T> object);

	//places rect within the region
	//if rect is larger than the region,
	//then rect is placed in it's centre
	template<typename T>
	rect_t<T> clamp_rect(rect_t<T> rect, rect_t<T> region);
}

#include "hades/detail/collision.inl"

#endif //HADES_COLLISION_HPP
