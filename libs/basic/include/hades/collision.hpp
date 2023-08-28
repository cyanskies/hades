#ifndef HADES_COLLISION_HPP
#define HADES_COLLISION_HPP

#include <tuple>
#include <vector>

#include "hades/poly_math.hpp"
#include "hades/rectangle_math.hpp"
#include "hades/types.hpp"
#include "hades/vector_math.hpp"

//collision detection functions for the following types
//rect_t
//point_t
//circle_t
//polygon_t

//algorithms work best with floating point types.

namespace hades
{
	template<typename T>
	using point_t = vector2<T>;
}

namespace hades::detail
{
	template<typename T>
	struct is_collidable : std::false_type {};

	template<typename T>
	struct is_collidable<rect_t<T>> : std::true_type {};

	template<typename T>
	struct is_collidable<point_t<T>> : std::true_type {};

	template<typename T>
	struct is_collidable<polygon_t<T>> : std::true_type {};

	template<typename T>
	constexpr auto is_collidable_v = is_collidable<T>::value;
}

namespace hades
{
	template<typename T>
	struct circle_t
	{
		T x, y, r;
	};

	/*template<typename T>
	struct polygon_t
	{
		vector2<T> position;
		std::vector<point_t<T>> vertex;
	};*/

	//TODO: collision traits

	//the following functions all support
	//supports:
	//point_t
	//rect_t
	//circle_t

	//returns true if the objects are intersecting
	template<typename U, typename V>
	bool collision_test(U first, V second);

	//NOTE: the layout of direction is depended on by other code.
	enum class direction {
		left,
		right,
		top,
		bottom,
		last
	};

	//returns the move needed to bring object as close as possible to other without colliding.
	//if return value == move, then no collision occured
	template<typename T, template<typename> typename U, template<typename> typename V>
	vector2<T> safe_move(U<T> object, vector2<T> move, V<T> other);
	
	template<typename T, typename Iter>
	struct collision_move_return
	{
		vector2<T> move;
		Iter other;
	};

	//as above, but accepts a container of collision objects to compare against
	//returns both the final move, and an iterator pointing to the colliding object
	// if no collision has been found, then it will return [move, end]
	template<typename T, template<typename> typename U, typename Iter>
	collision_move_return<T, Iter>
	safe_move(U<T> object, vector2<T> move, Iter begin, Iter end);

	//returns the move needed to bring object as close as possible to other without colliding.
	//uses as much of the movement vector as possible allowing sliding along walls, etc
	//if return value == move, then no collision occured
	//TODO: implement
	template<typename T, template<typename> typename U, template<typename> typename V>
	vector2<T> collision_move(U<T> object, vector2<T> move, V<T> other, T collision_friction = 0);

	//returns a vector normal from the point of collision
	template<typename T, template<typename> typename U, template<typename> typename V>
	vector2<T> collision_normal(U<T> object, vector2<T> move, V<T> other);

	//returns the rect side that collided
	template<typename T, template<typename> typename U>
	direction collision_direction(U<T> prev, U<T> current, rect_t<T> other);

	//generates a bounding box that at least covers the whole of the collision primative.
	template<typename T, template<typename> typename U>
	rect_t<T> bounding_box(U<T> object);

	//generates a circle that encompasses the whole object
	//template<typename T, template<typename> typename U>
	//rect_t<T> bounding_circle(U<T> object);

	//places rect within the region
	//if rect is larger than the region,
	//then rect is placed in it's centre
	template<typename T>
	constexpr rect_t<T> clamp_rect(rect_t<T> rect, rect_t<T> region) noexcept;
}

#include "hades/detail/collision.inl"

#endif //HADES_COLLISION_HPP
