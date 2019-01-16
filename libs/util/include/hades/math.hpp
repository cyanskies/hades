#ifndef HADES_UTIL_MATH_HPP
#define HADES_UTIL_MATH_HPP

#include <array>
#include <cassert>
#include <tuple>

#include "hades/types.hpp"
#include "hades/vector_math.hpp"

namespace hades
{
	template<typename T>
	using point_t = vector_t<T>;

	template<typename T>
	struct rect_t
	{
		constexpr rect_t() noexcept = default;
		constexpr rect_t(T x, T y, T width, T height) noexcept;
		constexpr rect_t(const vector_t<T> &pos, const vector_t<T> &siz) noexcept;

		T x{}, y{}, width{}, height{};
	};

	template<typename T>
	constexpr bool operator==(const rect_t<T> &lhs, const rect_t<T> &rhs);

	template<typename T>
	constexpr bool operator!=(const rect_t<T> &lhs, const rect_t<T> &rhs);

	using rect_int = rect_t<int32>;
	using rect_float = rect_t<float>;

	template<typename T>
	struct rect_centre_t
	{
		T x{}, y{}, half_width{}, half_height{};
	};

	using rect_centre_int = rect_centre_t<int32>;
	using rect_centre_float = rect_centre_t<float>;

	template<typename T>
	rect_t<T> to_rect(rect_centre_t<T>);

	template<typename T>
	rect_centre_t<T> to_rect_centre(rect_t<T>);

	template<typename T>
	vector_t<T> position(const rect_t<T>&);

	template<typename T>
	vector_t<T> size(const rect_t<T>&);

	template<typename T>
	bool intersects(const rect_t<T>&, const rect_t<T>&);

	template<typename T>
	bool intersect_area(const rect_t<T>&, const rect_t<T>&, rect_t<T> &area);

	template<typename T>
	rect_t<T> normalise(rect_t<T>);

	template<typename T>
	T clamp(T value, T min, T max);

	//NOTE: the layout of rect_coners is depended on by other code.
	enum class rect_corners : std::size_t {
		first,
		top_left = first,
		top_right,
		bottom_right,
		bottom_left,
		last
	};

	template<typename T>
	std::array<point_t<T>, 4> corners(rect_t<T>);

	template<typename T>
	const point_t<T> &get_corner(rect_corners, const std::array<point_t<T>, 4>&);

	template<typename T>
	point_t<T> &get_corner(rect_corners, std::array<point_t<T>, 4>&);

	//returns the distance between two points
	template<typename T>
	T distance(point_t<T> a, point_t<T> b);

	template<typename T>
	bool is_within(T value, T min, T max);

	template<typename T>
	bool is_within(point_t<T> value, rect_t<T> other);

	template<typename T>
	bool range_within(T first_min, T first_max, T second_min, T second_max);
}

#include "hades/detail/math.inl"

#endif // !HADES_UTIL_MATH_HPP