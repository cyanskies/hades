#include <array>
#include <cassert>

#include "hades/math.hpp"
#include "hades/types.hpp"
#include "hades/vector_math.hpp"

namespace hades
{
	//point tests
	template<typename T>
	bool collision_test(point_t<T> current, point_t<T> object)
	{
		//NOTE: point must return false, as any floating based point 
		//type will be impossible to compare for equality

		//point to point collisions aren't very usefull, 
		//so this shouldn't be a problem

		static_assert(always_false<T>::value, "Cannot test collisions between points");
		return false;
	}

	template<typename T>
	bool collision_test(point_t<T> current, rect_t<T> object)
	{
		return is_within(current.x, object.x, object.x + object.width)
			&& is_within(current.y, object.y, object.y + object.height);
	}

	template<typename T>
	bool collision_test(point_t<T> current, circle_t<T> object)
	{
		const auto distance = vector::distance(current, { object.x, object.y });
		return distance <= object.r;
	}

	template<typename T>
	bool collision_test(point_t<T> current, multipoint_t<T> object)
	{
		static_assert(always_false<T>::value, "Cannot test collisions between points");
		return false;
	}

	//rect tests
	template<typename T>
	bool collision_test(rect_t<T> lhs, point_t<T> rhs)
	{
		return collision_test(rhs, lhs);
	}

	template<typename T>
	bool collision_test(rect_t<T> lhs, rect_t<T> rhs)
	{
		return range_within(lhs.x, lhs.x + lhs.width, rhs.x, rhs.width)
			&& range_within(lhs.y, lhs.y + lhs.height, rhs.y, rhs.height);
	}

	template<typename T>
	bool collision_test(rect_t<T> lhs, circle_t<T> rhs)
	{
		//clamp the circles centre to a point within the rectangle
		const point_t<T> closest_point{ clamp(rhs.x, lhs.x, lhs.x + lhs.width),
										clamp(rhs.y, lhs.y, lhs.y + lhs.height) };

		//use point->circle collision detection
		return collision_test(closest_point, rhs);
	}

	//TODO:
	//rect to multipoint

	template<typename T>
	bool collision_test(circle_t<T> lhs, point_t<T> rhs)
	{
		return collision_test(rhs, lhs);
	}

	template<typename T>
	bool collision_test(circle_t<T> lhs, rect_t<T> rhs)
	{
		return collision_test(rhs, lhs);
	}

	template<typename T>
	bool collision_test(circle_t<T> lhs, circle_t<T> rhs)
	{
		const auto dist = vector::distance(lhs, rhs);
		return dist < lhs.r + rhs.r;
	}

	//TODO:
	//circle to multipoint

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

	template<typename T>
	std::tuple<bool, vector_t<T>> collision_test(point_t<T> prev, point_t<T> current, rect_t<T> other)
	{
		assert(!collision_test(prev, other));

		
		const vector_t<T> resolve{ current.x - prev.x, current.y - prev.y };

		
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

	namespace detail
	{
		template<typename T>
		T clamp_region(T x0, T w0, T x1, T w1)
		{
			if (w0 > w1)
			{
				const auto region_centre = w1 / 2;
				return region_centre - w0 / 2;
			}
			else if (x0 < x1)
				return x1;
			else if (x0 + w0 > x1 + x1)
				return x1 + w1 - w0;
			
			return x0;
		}
	}

	template<typename T>
	rect_t<T> clamp_rect(rect_t<T> rect, rect_t<T> region)
	{
		return { detail::clamp_region(rect.x, rect.width, region.x, region.width),
				 detail::clamp_region(rect.y, rect.height, region.y, region.height),
				 rect.width, rect.height };
	}
}