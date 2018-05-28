#include <cassert>

#include "hades/types.hpp"
#include "hades/vector_math.hpp"

namespace hades
{
	namespace detail
	{
		template<typename T>
		bool valid_rect(rect_t<T> r)
		{
			return r.width >= 0
				&& r.height >= 0;
		}

		template<typename T>
		bool valid_circle(circle_t<T> r)
		{
			return r.r >= 0;
		}
	}

	//point tests
	template<typename T>
	bool collision_test(point_t<T> current, point_t<T> object)
	{
		return current.x == object.x
			&& current.y == object.y;
	}

	template<typename T>
	bool collision_test(point_t<T> current, rect_t<T> object)
	{
		assert(detail::valid_rect(object));

		return current.x >= object.x
			&& current.x <= object.x + object.width
			&& current.y >= object.y
			&& current.y <= object.y + object.height;
	}

	//TODO:
	//point to circle
	//point to multipoint

	//rect tests
	template<typename T>
	bool collision_test(rect_t<T> lhs, point_t<T> rhs)
	{
		return collision_test(rhs, lhs);
	}

	template<typename T>
	bool collision_test(rect_t<T> lhs, rect_t<T> rhs)
	{
		assert(detail::valid_rect(lhs));
		assert(detail::valid_rect(rhs));

		const auto l_x2 = lhs.x + lhs.width;
		const auto l_y2 = lhs.y + lhs.height;

		const auto r_x2 = rhs.x + rhs.width;
		const auto r_y2 = rhs.y + rhs.height;

		return lhs.x < r_x2
			&& l_x2 > rhs.x
			&& lhs.x > r_y2
			&& l_y2 < rhs.y;
	}

	//TODO:
	//rect to circle
	//rect to multipoint
	//circle tests
	//multipoint tests

	template<typename U, typename V>
	bool collision_test(U current, V object)
	{
		static_assert(always_false<U, V>.value, "collision_test not defined for these types");
		return false;
	}

	//point tests
	template<typename T>
	std::tuple<bool, vector_t<T>> collision_test(point_t<T> prev, point_t<T> current, point_t<T> other)
	{
		assert(!collision_test(prev, other));

		const auto collide = collision_test(current, other);
		const vector_t<T> distance{ current.x - current.x / current.x, current.y - current.y / current.y };

		return { collide, distance };
	}

	//TODO:
	//point to rect
	//point to circle
	//point to multipoint
	//rect tests
	//rect to point
	//rect to rect
	//rect to circle
	//rect to multipoint
	//circle tests
	//multipoint tests

	template<typename T, template<typename> typename U, template<typename> typename V>
	std::tuple<bool, vector_t<T>, direction> collision_test(U<T> prev, U<T> current, V<T> object)
	{
		static_assert(always_false<T, U<T>, V<T>>.value, "collision_test not defined for these types");
		return { false, vector_t<T>{},  vector_t<T>{} };
	}

	//TODO:
	//all

	//returns the direction the collision occured from
	template<typename T, template<typename> typename U>
	direction collision_direction(U<T> prev, U<T> current, rect_t<T> other)
	{
		static_assert(always_false<T, U<T>, rect_t<T>>.value, "collision_direction not defined for these types");
		return direction::left;
	}

	//TODO:
	//all

	template<typename T, template<typename> typename U>
	rect_t<T> bounding_box(U<T> object)
	{
		static_assert(always_false<T, U<T>>.value, "bounding_box not defined for these types");
		return rect_t<T>{};
	}
}