#ifndef HADES_UTIL_MATH_HPP
#define HADES_UTIL_MATH_HPP

#include "hades/types.hpp"

namespace hades
{
	template<typename T>
	constexpr auto to_radians(T degrees) noexcept;

	template<std::floating_point Float>
	constexpr auto to_degrees(Float radians) noexcept;

	template<typename T>
		requires std::integral<T> || std::floating_point<T>
	[[nodiscard]] constexpr T normalise(T value, T start, T end) noexcept;

	template<std::floating_point T>
	[[nodiscard]] constexpr T remap(T value, T start, T end, T new_start, T new_end) noexcept;

	template<typename T>
	constexpr T clamp(T value, T min, T max) noexcept;

	template<typename T>
	bool is_within(T value, T min, T max);

	template<typename T>
	bool range_within(T first_min, T first_max, T second_min, T second_max);
}

#include "hades/detail/math.inl"

#endif // !HADES_UTIL_MATH_HPP
