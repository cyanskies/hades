#ifndef HADES_UTIL_RECT_MATH_HPP
#define HADES_UTIL_RECT_MATH_HPP

#include <array>
#include <type_traits>

#include "hades/types.hpp"
#include "hades/math.hpp"

namespace hades
{
	template<typename T>
	struct rect_t
	{
		using value_type = T;

		constexpr rect_t() noexcept = default;
		constexpr rect_t(T x, T y, T width, T height) noexcept;
		constexpr rect_t(const vector_t<T> &pos, const vector_t<T> &siz) noexcept;

		template<typename U>
		explicit constexpr operator rect_t<U>() const noexcept;

		T x, y, width, height;
	};

	static_assert(std::is_trivial_v<rect_t<int32>>);

	template<typename T>
	constexpr bool operator==(const rect_t<T> &lhs, const rect_t<T> &rhs);

	template<typename T>
	constexpr bool operator!=(const rect_t<T> &lhs, const rect_t<T> &rhs);

	template<typename T>
	constexpr bool operator<(const rect_t<T>& lhs, const rect_t<T>& rhs) noexcept;

	using rect_int = rect_t<int32>;
	using rect_float = rect_t<float>;

	template<typename T>
	struct rect_centre_t
	{
		T x, y, half_width, half_height;
	};

	static_assert(std::is_trivial_v<rect_centre_t<int32>>);

	using rect_centre_int = rect_centre_t<int32>;
	using rect_centre_float = rect_centre_t<float>;

	template<typename T>
	constexpr rect_t<T> to_rect(rect_centre_t<T>) noexcept;

	template<typename T>
	constexpr rect_centre_t<T> to_rect_centre(rect_t<T>) noexcept;

	template<typename T>
	constexpr vector_t<T> position(const rect_t<T>&) noexcept;

	template<typename T>
	constexpr vector_t<T> size(const rect_t<T>&) noexcept;

	template<typename T>
	constexpr bool aabb_test(const rect_t<T>&, const rect_t<T>&) noexcept;

	template<typename T>
	constexpr bool intersects(const rect_t<T>&, const rect_t<T>&) noexcept;

	template<typename T>
	constexpr bool intersect_area(const rect_t<T>&, const rect_t<T>&, rect_t<T> &area) noexcept;

	template<typename T>
	constexpr rect_t<T> normalise(rect_t<T>) noexcept;

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
	constexpr std::array<point_t<T>, 4> corners(rect_t<T>) noexcept;

	template<typename T>
	constexpr const point_t<T> &get_corner(rect_corners, const std::array<point_t<T>, 4>&) noexcept;

	template<typename T>
	constexpr point_t<T> &get_corner(rect_corners, std::array<point_t<T>, 4>&) noexcept;

	template<typename T>
	constexpr bool is_within(point_t<T> value, rect_t<T> other) noexcept;

	//returns true if the first rectangle is entirely within the second
	template<typename T>
	constexpr bool is_within(rect_t<T> first, rect_t<T> second) noexcept;
}

#include "hades/detail/rectangle_math.inl"

#endif // !HADES_UTIL_RECT_MATH_HPP
