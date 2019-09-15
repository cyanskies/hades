#include "hades/rectangle_math.hpp"

#include <algorithm> // std::min/max
#include <cassert>
#include <cmath> // std::abs

namespace hades
{
	template<typename T>
	constexpr inline rect_t<T>::rect_t(T x, T y, T width, T height) noexcept
		: x{ x }, y{ y }, width{ width }, height{ height } {}

	template<typename T>
	constexpr inline rect_t<T>::rect_t(const vector_t<T> &pos, const vector_t<T> &siz) noexcept
		: x{ pos.x }, y{ pos.y }, width{ siz.x }, height{ siz.y } {}

	template<typename T>
	constexpr bool operator==(const rect_t<T> &lhs, const rect_t<T> &rhs)
	{
		return lhs.x == rhs.x &&
			lhs.y == rhs.y &&
			lhs.height == rhs.height &&
			lhs.width == rhs.width;
	}

	template<typename T>
	constexpr bool operator!=(const rect_t<T> &lhs, const rect_t<T> &rhs)
	{
		return !(lhs == rhs);
	}

	template<typename T>
	constexpr bool operator<(const rect_t<T>& l, const rect_t<T>& r) noexcept
	{
		return std::tie(l.x, l.y, l.width, l.height) <
			std::tie(r.x, r.y, r.width, r.height);
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
	vector_t<T> position(const rect_t<T> &r)
	{
		return { r.x, r.y };
	}

	template<typename T>
	vector_t<T> size(const rect_t<T> &r)
	{
		return { r.width, r.height };
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
	std::array<point_t<T>, 4> corners(rect_t<T> r)
	{
		return {
			point_t<T>{r.x, r.y},
			point_t<T>{r.x + r.width, r.y},
			point_t<T>{r.x + r.width, r.y + r.height},
			point_t<T>{r.x, r.y + r.height}
		};
	}

	namespace detail
	{
		template<typename T>
		auto &get_corner_impl(rect_corners c, T &a)
		{
			static_assert(std::is_array_v<T>);
			static_assert(a.size() == 4);
			assert(c >= rect_corners::first);
			assert(c < rect_corners::last);
			const auto index = static_cast<std::decay_t<decltype(a)>::size_type>(c);
			return a[index];
		}
	}

	template<typename T>
	const point_t<T> &get_corner(rect_corners c, const std::array<point_t<T>, 4> &a)
	{
		return detail::get_corner_impl(c, a);
	}

	template<typename T>
	point_t<T> &get_corner(rect_corners c, std::array<point_t<T>, 4> &a)
	{
		return detail::get_corner_impl(c, a);
	}

	template<typename T>
	bool is_within(point_t<T> value, rect_t<T> other)
	{
		return is_within(value.x, other.x, other.x + other.width)
			&& is_within(value.y, other.y, other.y + other.height);
	}

	template<typename T>
	bool is_within(rect_t<T> first, rect_t<T> second)
	{
		auto area = rect_t<T>{};
		intersect_area(first, second, area);

		if constexpr(std::is_floating_point_v<T>)
		{
			return float_near_equal(first.width, area.width)
				&& float_near_equal(first.height, area.height);
		}
		else
			return size(first) == size(area);
	}
}