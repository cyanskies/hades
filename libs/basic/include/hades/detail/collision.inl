#include "hades/collision.hpp"

#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

#include "hades/line_math.hpp"
#include "hades/math.hpp"
#include "hades/types.hpp"
#include "hades/vector_math.hpp"

namespace hades
{
	namespace detail
	{
		template<typename T>
		std::tuple<point_t<T>, point_t<T>> line_outside_inside(point_t<T> start, point_t<T> end, rect_t<T> other)
		{
			if (collision_test(start, other))
				return { end, start };

			return { start, end };
		}

		//returns the side of the rectangle that was hit, and the point of the intersection
		template<typename T>
		std::tuple<direction, point_t<T>> line_collision(point_t<T> start, point_t<T> end, rect_t<T> other)
		{
			//FIXME: the collision is jittery
			const auto[outside, inside] = line_outside_inside(start, end, other);

			assert(inside != outside);
			const auto lines = line::from_rect(other);

			//line value type
			using line_val = std::tuple<T, line_t<T>, direction>;
			std::vector<line_val> sorted_lines;
			
			for (auto i = 0; i < static_cast<int>(direction::last); ++i)
			{
				const auto l = lines[i];
				const point_t<T> midpoint{ (l.s.x + l.e.x) / 2,
									(l.s.y + l.e.y) / 2 };
				sorted_lines.emplace_back(vector::distance(outside, midpoint), l, static_cast<direction>(i));
			}

			std::sort(std::begin(sorted_lines), std::end(sorted_lines), [](auto &&lhs, auto &&rhs) {
				return std::get<T>(lhs) < std::get<T>(rhs);
			});

			const line_t<T> approach{ inside, outside };
			const auto approach_rect = bounding_box(approach);

			//find the intersect point
			for (const auto &l : sorted_lines)
			{
				const auto i = line::intersect(approach, std::get<line_t<T>>(l));
				if (i && is_within(*i, approach_rect))
					return { std::get<direction>(l), *i };
			}

			return { direction::last, outside };
		}
	}

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

	//rect tests
	template<typename T>
	bool collision_test(rect_t<T> lhs, point_t<T> rhs)
	{
		return collision_test(rhs, lhs);
	}

	template<typename T>
	bool collision_test(rect_t<T> lhs, rect_t<T> rhs)
	{
		return range_within(lhs.x, lhs.x + lhs.width, rhs.x, rhs.x + rhs.width)
			&& range_within(lhs.y, lhs.y + lhs.height, rhs.y, rhs.y + rhs.height);
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

	template<typename U, typename V>
	bool collision_test(U current, V object)
	{
		static_assert(always_false<U, V>::value, "collision_test not defined for these types");
		return false;
	}

	//point
	template<typename T>
	vector_t<T> safe_move(point_t<T> prev, vector_t<T> move, point_t<T> other)
	{
		if (collision_test(prev + move, other))
			return move - vector_t<T>{1, 1};

		return move;
	}

	template<typename T>
	vector_t<T> safe_move(point_t<T> prev, vector_t<T> move, rect_t<T> other)
	{
		if (!collision_test(prev + move, other))
			return move;

		const auto [direc, intersect] = detail::line_collision(prev, prev + move, other);
		if (direc == direction::last)
			return move;

		const auto intersect_vector = intersect - prev;
		const auto move_mag = vector::magnitude(move);
		const auto inter_mag = vector::magnitude(intersect_vector) - 1;
		return vector::resize(move, std::min(move_mag, inter_mag ));
	}

	template<typename T>
	vector_t<T> safe_move(point_t<T> prev, vector_t<T> move, circle_t<T> other)
	{
		return safe_move(circle_t<T>{prev.x, prev.y, 0}, move, other);
	}

	//rect
	template<typename T>
	vector_t<T> safe_move(rect_t<T> object, vector_t<T> move, point_t<T> other)
	{
		return safe_move(object, move, circle_t<T>{other.x, other.y, 1});
	}

	template<typename T>
	vector_t<T> safe_move(rect_t<T> object, vector_t<T> move, rect_t<T> other)
	{
		object = normalise(object);
		other = normalise(other);

		const rect_t<T> moved_object{ 
			object.x + move.x, 
			object.y + move.y, 
			object.width, 
			object.height 
		};

		if (!collision_test(moved_object, other))
			return move;

		const auto object_centre = to_rect_centre(object);
		const auto other_centre = to_rect_centre(other);
	
		const vector_t<T> other_super_size{ 
			other_centre.half_width + object_centre.half_width,
			other_centre.half_height + object_centre.half_height
		};

		const rect_centre_t<T> other_expanded{ 
			other_centre.x,
			other_centre.y,
			other_super_size.x,
			other_super_size.y
		};
	
		const auto super_size_other = to_rect(other_expanded);

		return safe_move(point_t<T>{ object_centre.x, object_centre.y }, move, super_size_other);
	}

	template<typename T>
	vector_t<T> safe_move(rect_t<T> object, vector_t<T> move, circle_t<T> other)
	{
		return vector::reverse(safe_move(other, vector::reverse(move), object));
	}

	//circle
	template<typename T>
	vector_t<T> safe_move(circle_t<T> object, vector_t<T> move, point_t<T> other)
	{
		return safe_move(object, move, circle_t<T>{other.x, other.y, 0});
	}

	template<typename T>
	vector_t<T> safe_move(circle_t<T> object, vector_t<T> move, rect_t<T> other)
	{
		const point_t<T> closest_point{ clamp(object.x, other.x, other.x + other.width),
			clamp(object.y, other.y, other.y + other.height) };

		return safe_move(object, move, closest_point);
	}

	template<typename T>
	vector_t<T> safe_move(circle_t<T> object, vector_t<T> move, circle_t<T> other)
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

	template<typename T, template<typename> typename U, template<typename> typename V>
	vector_t<T> safe_move(U<T> object, vector_t<T> move, V<T> other)
	{
		static_assert(always_false<T, U<T>, V<T>>::value, "safe_move not defined for these types");
		return move;
	}

	template<typename T, template<typename> typename U, template<typename> typename V>
	vector_t<T> collision_move(U<T> object, vector_t<T> move, V<T> other)
	{
		static_assert(always_false<T, U<T>, V<T>>::value, "collision_move not defined for these types");
		return move;
	}

	template<typename T, template<typename> typename U>
	direction collision_direction(point_t<T> object, U<T> move, rect_t<T> other)
	{
		const auto moved_object = object + move;
		if (!collision_test(moved_object, other))
			return direction::last;

		const auto ret = detail::line_collision(object, moved_object, other);

		return std::get<direction>(ret);
	}
	
	template<typename T, template<typename> typename U>
	direction collision_direction(rect_t<T> object, U<T> move, rect_t<T> other)
	{
		object = normalise(object);
		other = normalise(other);

		const rect_t<T> moved_object{
			object.x + move.x,
			object.y + move.y,
			object.width,
			object.height
		};

		if (!collision_test(moved_object, other))
			return direction::last;

		const auto object_centre = to_rect_centre(object);
		const auto other_centre = to_rect_centre(other);

		return detail::line_collision({ object_centre.x, object_centre.y }, { other_centre.x, other_centre.y }, other);
	}

	template<typename T, template<typename> typename U>
	direction collision_direction(circle_t<T> object, U<T> move, rect_t<T> other)
	{
		const point_t<T> moved_object{ object.x + move.x, object.y + move.y };
		const point_t<T> closest_point{ clamp(moved_object.x, other.x, other.x + other.width),
			clamp(moved_object.y, other.y, other.y + other.height) };

		const auto ret = detail::line_collision({ object.x, object.y }, closest_point, other);

		return std::get<direction>(ret);
	}

	//returns the direction the collision occured from
	template<typename T, template<typename> typename U>
	direction collision_direction(U<T> object, U<T> move, rect_t<T> other)
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

	template<typename T>
	rect_t<T> bounding_box(line_t<T> object)
	{
		const auto x1 = std::min(object.s.x, object.e.x);
		const auto x2 = std::max(object.s.x, object.e.x);
		const auto y1 = std::min(object.s.y, object.e.y);
		const auto y2 = std::max(object.s.y, object.e.y);

		return {x1, y1, x2 - x1, y2 - y1};
	}

	template<typename T, template<typename> typename U>
	rect_t<T> bounding_box(U<T> object)
	{
		static_assert(always_false<T, U<T>>::value, "bounding_box not defined for these types");
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