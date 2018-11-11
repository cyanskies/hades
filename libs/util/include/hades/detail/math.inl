#include "hades/math.hpp"

#include <algorithm>

namespace hades
{
	template<typename T>
	bool operator==(const rect_t<T> &lhs, const rect_t<T> &rhs)
	{
		return lhs.x == rhs.x,
			lhs.y == rhs.y,
			lhs.height == rhs.height,
			lhs.width == rhs.width;
	}

	template<typename T>
	bool operator!=(const rect_t<T> &lhs, const rect_t<T> &rhs)
	{
		return !(lhs == rhs);
	}

	template<typename T>
	rect_t<T> to_rect(rect_centre_t<T> r)
	{
		return {
			r.x - r.half_width,
			r.y - r.half_height,
			r.half_width * 2,
			r.half_height * 2
		};
	}

	template<typename T>
	rect_centre_t<T> to_rect_centre(rect_t<T> r)
	{
		r = normalise(r);
		const vector_t<T> half_size{ r.width / 2, r.height / 2 };
		return { r.x + half_size.x, r.y + half_size.y, half_size.x, half_size.y };
	}

	template<typename T>
	bool intersects(const rect_t<T> &l, const rect_t<T> &r)
	{
		return intersect_area(l, r, rect_t<T>{});
	}

	template<typename T>
	bool intersect_area(const rect_t<T> &l, const rect_t<T> &r, rect_t<T> &a)
	{
		// Rectangles with negative dimensions are allowed, so we must handle them correctly
		const auto l_normal = normalise(l);
		const auto r_normal = normalise(r);

		// Compute the intersection boundaries
		const auto left   = std::max(l_normal.x, r_normal.x);
		const auto top    = std::max(l_normal.y, r_normal.y);
		const auto right  = std::min(l_normal.x + l_normal.width, r_normal.x + r_normal.width);
		const auto bottom = std::min(l_normal.y + l_normal.height, r_normal.y + r_normal.height);

		if ((left < right) && (top < bottom))
		{
			a = { left, top, right - left, bottom - top };
			return true;
		}

		a = {};
		return false;
	}

	template<typename T>
	rect_t<T> normalise(rect_t<T> r)
	{
		return {
			std::min(r.x, r.x + r.width),
			std::min(r.y, r.y + r.height),
			std::abs(r.width),
			std::abs(r.height)
		};
	}

	template<typename T>
	T clamp(T value, T min, T max)
	{
		return std::clamp(value, std::min(min, max), std::max(min, max));
	}

	template<typename T>
	std::array<point_t<T>, 4> corners(rect_t<T> r)
	{
		return {
			point_t<T>{r.x, r.y},
			point_t<T>{r.x + r.width, r.y},
			point_t<T>{r.x + r.width, r.y + r.height},
			point_t<T>{r.x, r.y + r.height}
		};
	}

	template<typename T>
	T distance(point_t<T> a, point_t<T> b)
	{
		const auto x = b.x - a.x;
		const auto y = b.y - a.y;
		return vector::magnitude({ x, y });
	}

	template<typename T>
	bool is_within(T value, T min, T max)
	{
		return value >= std::min(min, max) && value <= std::max(min, max);
	}

	template<typename T>
	bool is_within(point_t<T> value, rect_t<T> other)
	{
		return is_within(value.x, other.x, other.x + other.width)
			&& is_within(value.y, other.y, other.y + other.height);
	}

	template<typename T>
	bool range_within(T first_min, T first_max, T second_min, T second_max)
	{
		return std::max(first_min, first_max) >= std::min(second_min, second_max)
			&& std::min(first_min, first_max) <= std::max(second_min, second_max);
	}
}