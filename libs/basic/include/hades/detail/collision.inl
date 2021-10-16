#include "hades/collision.hpp"

#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

#include "hades/exceptions.hpp"
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

			assert(inside != outside && "both line ends are the same, this is probably the same line being compared to itself");
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
		if constexpr (std::is_floating_point_v<T>)
			return float_near_equal(current, object);
		else
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
		const auto ab = current - vector_t<T>{ object.x, object.y };
		const auto mag = vector::magnitude_squared(ab);
		return mag < object.r * object.r;
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
		const auto d = lhs.r + rhs.r;
		return dist2 < d * d;
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
		if (prev == prev + move)
			return move;

		if (collision_test(prev, other))
			return { 0, 0 };

		if (!collision_test(prev + move, other))
			return move;

		const auto [direc, intersect] = detail::line_collision(prev, prev + move, other);
		if (direc == direction::last)
			return move;

		const auto intersect_vector = intersect - prev;
		const auto move_mag = vector::magnitude(move);
		const auto inter_mag = vector::magnitude(intersect_vector) - 1;
		//if the -1 brought intersect_magnitude below 0, then clamp it to 0
		return vector::resize(move, std::min(move_mag, inter_mag < 0 ? 0 : inter_mag ));
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
		const auto d = object.r + other.r;
		const auto rad2 = d * d;

		//early exit if collision is impossible.
		if (vector::magnitude_squared(move) <
			vector::magnitude_squared(obj_to_oth) - rad2)
			return move;

		const auto move_unit = vector::unit(move);
		const auto dot = vector::dot(move_unit, obj_to_oth);

		if (dot < 0)
			return move;

		const auto obj_to_oth_len2 = vector::magnitude_squared(obj_to_oth);
		const auto f = obj_to_oth_len2 - (dot * dot);

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

	template<typename T, template<typename> typename U, typename Iter>
	collision_move_return<T, Iter>
		safe_move(U<T> object, vector_t<T> move, Iter iter, Iter end)
	{
		//calculate move for each member of others
		//return the shortest move, along with the index for that member
		auto move_out = move;
		auto move_mag = vector::magnitude_squared(move);
		auto iter_out = end;
		for (iter; iter != end; ++iter)
		{
			const auto this_move = safe_move(object, move, *iter);
			const auto this_mag = vector::magnitude_squared(this_move);
			if (this_mag < move_mag)
			{
				move_out = this_move;
				move_mag = this_mag;
				iter_out = iter;
			}
		}

		return { move_out, iter_out };
	}

	template<typename T, template<typename> typename U, template<typename> typename V>
	vector_t<T> collision_move(U<T> object, vector_t<T> move, V<T> other, T friction)
	{
		static_assert(always_false<T, U<T>, V<T>>::value, "collision_move not defined for these types");
		return move;
	}


	template<typename T>
	vector_t<T> collision_normal(rect_t<T> object, vector_t<T> move, rect_t<T> other)
	{
		const auto direc = collision_direction(object, move, other);
		switch (direc)
		{
		case direction::left:
			return { -1, 0 };
		case direction::right:
			return { 1, 0 };
		case direction::top:
			return {0, -1};
		case direction::bottom:
			return {0, 1};
		}

		throw runtime_error{ "bad rectangle collision calculation" };
	}

	template<typename T, template<typename> typename U, template<typename> typename V>
	vector_t<T> collision_normal(U<T> object, vector_t<T> move, V<T> other)
	{
		static_assert(always_false_v<U<T>, T, V<T>>, "collision incident hasn't been defined for these types");
		return vector_t<T>{};
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

		const auto ret = detail::line_collision({ object_centre.x, object_centre.y }, { other_centre.x, other_centre.y }, other);
		return std::get<direction>(ret);
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
		constexpr T clamp_region(T x0, T w0, T x1, T w1) noexcept
		{
			if (w0 > w1)
			{
				const auto region_centre = w1 / 2;
				return region_centre - w0 / 2;
			}
			else if (x0 < x1)
				return x1;
			else if (x0 + w0 > x1 + w1)
				return x1 + w1 - w0;
			
			return x0;
		}
	}

	template<typename T>
	constexpr rect_t<T> clamp_rect(rect_t<T> rect, rect_t<T> region) noexcept
	{
		return { detail::clamp_region(rect.x, rect.width, region.x, region.width),
				 detail::clamp_region(rect.y, rect.height, region.y, region.height),
				 rect.width, rect.height };
	}
}
