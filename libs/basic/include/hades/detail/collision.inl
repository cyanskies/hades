#include "hades/collision.hpp"

#include <array>
#include <cassert>
#include <cmath>

#include "hades/line_math.hpp"
#include "hades/math.hpp"
#include "hades/types.hpp"
#include "hades/vector_math.hpp"

namespace hades
{
	//point tests
	template<typename T>
	bool collision_test(point_t<T> current, point_t<T> object)
	{
		//NOTE: 
		//type will be impossible to compare for equality

		//point to point collisions aren't very usefull, 
		//so this shouldn't be a problem
		return current == object;
	}

	template<typename T>
	bool collision_test(point_t<T> current, rect_t<T> object)
	{
		return is_within(current, object);
	}

	template<typename T>
	bool collision_test(point_t<T> current, circle_t<T> object)
	{
		const auto distance = vector::distance(current, { object.x, object.y });
		return distance < object.r;
	}

	template<typename T>
	bool collision_test(point_t<T> current, multipoint_t<T> object)
	{
		return std::any_of(std::begin(object.points), std::end(object.points), [current](auto &&p) {
			return collision_test(current, p);
		});
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
		const auto dist2 = vector::magnitude_squared<T>(vector_t<T>{ lhs.x, lhs.y } - vector_t<T>{ rhs.x, rhs.y });
		return dist2 < std::pow(lhs.r + rhs.r, 2);
	}

	//TODO:
	//circle to multipoint

	//multipoint tests

	template<typename U, typename V>
	bool collision_test(U current, V object)
	{
		static_assert(always_false<U, V>::value, "collision_test not defined for these types");
		return false;
	}

	//point tests
	template<typename T>
	std::tuple<bool, vector_t<T>> collision_test(point_t<T> prev, point_t<T> current, point_t<T> other)
	{
		static_assert(always_false<T>::value, "Cannot test collisions between points");
		return { false, vector_t<T>{} };
	}

	template<typename T>
	std::tuple<bool, vector_t<T>> collision_test(point_t<T> prev, point_t<T> current, rect_t<T> other)
	{
		assert(!collision_test(prev, other));

		const auto col = collision_test(current, other);
		const auto lines = line::from_rect(r);

		const auto approach = prev - current;
		auto approach_rect = bounding_box(approach);
		approach_rect.x = current.x;
		approach_rect.y = current.y;

		point_t<T> intersect{};
		bool found = false;
		//find the intersect point
		for (const auto &l : lines)
		{
			const auto i = line::intersect(approach, l);
			if (is_within(i, approach_rect))
			{
				assert(!found);
				found = true;
				intersect = i;
			}
		}

		return { col, current - intersect };
	}

	//TODO:
	//point to circle
	//point to multipoint
	//rect tests
	template<typename T>
	vector_t<T> collision_move(rect_t<T> object, vector_t<T> move, point_t<T> other)
	{
		return collision_move(object, move, circle_t<T>{other.x, other.y, 1});
	}
	
	template<typename T>
	vector_t<T> collision_move(rect_t<T> object, vector_t<T> move, rect_t<T> other)
	{
		const rect_t<T> moved_rect{ object.x + move.x, object.y + move.y, object.width, object.height };
		const auto intersect = rect_intersection(moved_rect, other);
		if (intersect.width >= 0 && intersect.height >= 0)
			return move - vector_t<T>{ intersect.width, intersect.height };

		return move;
	}

	template<typename T>
	vector_t<T> collision_move(rect_t<T> object, vector_t<T> move, circle_t<T> other)
	{
		return vector::reverse(collision_move(other, vector::reverse(move), object));
	}

	//rect to multipoint

	//circle tests
	template<typename T>
	vector_t<T> collision_move(circle_t<T> object, vector_t<T> move, point_t<T> other)
	{
		return collision_move(object, move, circle_t<T>{other.x, other.y, 0});
	}

	template<typename T>
	vector_t<T> collision_move(circle_t<T> object, vector_t<T> move, rect_t<T> other)
	{
		const point_t<T> closest_point{ clamp(object.x, other.x, other.x + other.width),
			clamp(object.y, other.y, other.y + other.height) };

		return collision_move(object, move, closest_point);
	}

	template<typename T>
	vector_t<T> collision_move(circle_t<T> object, vector_t<T> move, circle_t<T> other)
	{
		//based off article here:
		//https://www.gamasutra.com/view/feature/131424/pool_hall_lessons_fast_accurate_.php?print=1

		const vector_t<T> obj_pos{ object.x, object.y };
		const vector_t<T> oth_pos{ other.x, other.y };
		const auto obj_to_oth = oth_pos - obj_pos;
		const auto rad2 = std::pow(object.r + other.r, 2);

		//early exit if collision is impossible.
		if (vector::magnitude_squared(move) <
			vector::magnitude_squared(obj_to_oth) - rad2)
			return move;

		const auto move_unit = vector::unit(move);
		const auto dot = vector::dot(move_unit, obj_to_oth);
		
		if (dot < 0)
			return move;

		const auto obj_to_oth_len2 = vector::magnitude_squared(obj_to_oth);
		const auto f = obj_to_oth_len2 - std::pow(dot, 2);

		if (f >= rad2)
			return move;

		const auto t = rad2 - f;

		if (t < 0)
			return move;

		const auto distance = dot - std::sqrt(t);

		if (vector::magnitude(move) < distance)
			return move;

		return  vector::resize(move, distance);
	}
	//circel to multipoint
	//multipoint tests

	template<typename T, template<typename> typename U, template<typename> typename V>
	vector_t<T> collision_move(U<T> object, vector_t<T> move, V<T> other)
	{
		static_assert(always_false<T, U<T>, V<T>>::value, "collision_move not defined for these types");
		return move;
	}

	//TODO:
	//all

	//returns the direction the collision occured from
	template<typename T, template<typename> typename U>
	direction collision_direction(U<T> prev, U<T> current, rect_t<T> other)
	{
		static_assert(always_false<T, U<T>, rect_t<T>>::value, "collision_direction not defined for these types");
		return direction::left;
	}

	template<typename T>
	rect_t<T> rect_intersection(rect_t<T> lhs, rect_t<T> rhs)
	{
		const auto r1minx = std::min(lhs.x, lhs.x + lhs.width);
		const auto r1maxx = std::max(lhs.x, lhs.x + lhs.width);
		const auto r1miny = std::min(lhs.y, lhs.y + lhs.height);
		const auto r1maxy = std::max(lhs.y, lhs.y + lhs.height);

		const auto r2minx = std::min(rhs.x, rhs.x + rhs.width);
		const auto r2maxx = std::max(rhs.x, rhs.x + rhs.width);
		const auto r2miny = std::min(rhs.y, rhs.y + rhs.height);
		const auto r2maxy = std::max(rhs.y, rhs.y + rhs.height);

		const auto i_x_min = std::max(r1minx, r2minx);
		const auto i_x_max = std::min(r1maxx, r2maxx);
		const auto i_y_min = std::max(r1miny, r2miny);
		const auto i_y_max = std::min(r1maxy, r2maxy);

		return { i_x_min, i_y_min, i_x_max - i_x_min, i_y_max - i_y_min };
	}

	//TODO:
	//all
	template<typename T>
	rect_t<T> bounding_box(vector_t<T> object)
	{
		rect_t<T> r{};
		r.width = object.x;
		r.height = object.y;
		return r;
	}

	template<typename T>
	rect_t<T> bounding_box(rect_t<T> object)
	{
		return object;
	}

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