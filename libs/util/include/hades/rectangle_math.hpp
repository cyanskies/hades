#ifndef HADES_UTIL_RECT_MATH_HPP
#define HADES_UTIL_RECT_MATH_HPP

#include <array>
#include <format>
#include <type_traits>

#include "hades/types.hpp"
#include "hades/math.hpp"
#include "hades/vector_math.hpp"

namespace hades
{
	template<typename T>
	struct rect_t
	{
		using value_type = T;

		constexpr rect_t() noexcept = default;
		constexpr rect_t(T x, T y, T width, T height) noexcept;
		constexpr rect_t(const vector2<T>& pos, const vector2<T>& siz) noexcept;

		template<typename U>
		explicit constexpr operator rect_t<U>() const noexcept;

		T& operator[](std::size_t i) noexcept
		{
			constexpr auto arr = std::array{ &rect_t::x, &rect_t::y, &rect_t::width, &rect_t::height };
			assert(i < size(arr));
			return std::invoke(arr[i], this);
		}

		const T& operator[](std::size_t i) const noexcept
		{
			constexpr auto arr = std::array{ &rect_t::x, &rect_t::y, &rect_t::width, &rect_t::height };
			assert(i < size(arr));
			return std::invoke(arr[i], this);
		}

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

	//this is buggy for integer types
	template<typename T>
	struct rect_centre_t
	{
		T x, y, half_width, half_height;
	};

	static_assert(std::is_trivial_v<rect_centre_t<int32>>);

	using rect_centre_float = rect_centre_t<float>;

	template<typename T>
	constexpr rect_t<T> to_rect(rect_centre_t<T>) noexcept;

	template<typename T>
	constexpr rect_centre_t<T> to_rect_centre(rect_t<T>) noexcept;

	template<typename T>
	constexpr vector2<T> position(const rect_t<T>&) noexcept;

	template<typename T>
	constexpr vector2<T> size(const rect_t<T>&) noexcept;

	template<typename T>
	constexpr bool aabb_test(const rect_t<T>&, const rect_t<T>&) noexcept;

	template<typename T>
	constexpr bool intersects(const rect_t<T>&, const rect_t<T>&) noexcept;

	template<typename T>
	constexpr bool intersect_area(const rect_t<T>&, const rect_t<T>&, rect_t<T> &area) noexcept;

	template<typename T>
	constexpr rect_t<T> normalise(rect_t<T>) noexcept;

	//NOTE: the layout of rect_coners and rect_edges is depended on by other code.
	enum class rect_corners : std::uint8_t {
		first,
		top_left = first,
		top_right,
		bottom_right,
		bottom_left,
		last
	};

	enum class rect_edges : std::uint8_t {
		top,
		begin = top,
		right,
		bottom,
		left,
		uphill,
		downhill,
		end
	};

	enum class [[deprecated]] rect_diag : std::uint8_t {
		uphill = rect_edges::uphill,
		begin = uphill,
		downhill,
		end
	};

	template<typename T>
	constexpr std::array<vector2<T>, 4> corners(rect_t<T>) noexcept;

	template<typename T>
	constexpr const vector2<T> &get_corner(rect_corners, const std::array<vector2<T>, 4>&) noexcept;

	template<typename T>
	constexpr vector2<T> &get_corner(rect_corners, std::array<vector2<T>, 4>&) noexcept;

	template<typename T>
	constexpr bool is_within(vector2<T> value, rect_t<T> other) noexcept;

	//returns true if the first rectangle is entirely within the second
	template<typename T>
	constexpr bool is_within(rect_t<T> first, rect_t<T> second) noexcept;
}

template<typename CharT>
struct std::formatter<hades::rect_edges, CharT>;

#include "hades/detail/rectangle_math.inl"

#endif // !HADES_UTIL_RECT_MATH_HPP
