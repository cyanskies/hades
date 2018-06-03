#ifndef HADES_COLLISION_HPP
#define HADES_COLLISION_HPP

#include <tuple>
#include <vector>

#include "hades/math.hpp"
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

	//TODO: right angle triangles

	//the following functions all support
	//supports:
	//point_t
	//rect_t
	//circle_t
	//triangle_t

	//returns true if the objects are intersecting
	//supports:
	//point_t
	//rect_t
	//circle_t
	//triangle_t
	template<typename U, typename V>
	bool collision_test(U first, V second);

	enum class direction {
		left,
		right,
		top,
		bottom
	};

	//returns the move needed to bring object as close as possible to other without colliding.
	//if return value == move, then no collision occured
	//supports:
	//circle_t
	//rectangle_t
	//triangle_t
	template<typename T, template<typename> typename U, template<typename> typename V>
	vector_t<T> safe_move(U<T> object, vector_t<T> move, V<T> other);
	
	//returns the move needed to bring object as close as possible to other without colliding.
	//uses as much of the movement vector as possible allowing sliding along walls, etc
	//if return value == move, then no collision occured
	//supports:
	//circle_t
	//rectangle_t
	//triangle_t
	template<typename T, template<typename> typename U, template<typename> typename V>
	vector_t<T> collision_move(U<T> object, vector_t<T> move, V<T> other);

	//returns the direction the collision occured from
	template<typename T, template<typename> typename U>
	direction collision_direction(U<T> prev, U<T> current, rect_t<T> other);

	template<typename T>
	rect_t<T> rect_intersection(rect_t<T> lhs, rect_t<T> rhs);

	//generates a bounding box that at least covers the whole of the collision primative.
	template<typename T, template<typename> typename U>
	rect_t<T> bounding_box(U<T> object);

	//generates a circle that encompasses the whole object
	template<typename T, template<typename> typename U>
	rect_t<T> bounding_circle(U<T> object);

	//places rect within the region
	//if rect is larger than the region,
	//then rect is placed in it's centre
	template<typename T>
	rect_t<T> clamp_rect(rect_t<T> rect, rect_t<T> region);
}

#include "hades/detail/collision.inl"

#endif //HADES_COLLISION_HPP
