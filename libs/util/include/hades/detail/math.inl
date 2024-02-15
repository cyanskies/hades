#include "hades/math.hpp"

#include <algorithm>
#include <cassert>
#include <numbers>

namespace hades
{
	template<typename T>
	constexpr auto to_radians(T degrees) noexcept
	{
		constexpr auto ratio = std::numbers::pi_v<std::conditional_t<std::floating_point<T>, T, float>> / 180;
		return degrees * ratio;
	}

	template<std::floating_point Float>
	constexpr auto to_degrees(Float radians) noexcept
	{
		constexpr auto ratio = 180.f / std::numbers::pi_v<Float>;
		auto degs = radians * ratio;
		if (degs < 0)
			return degs + 360;
		return degs;
	}

	template<typename T>
		requires std::integral<T> || std::floating_point<T>
	constexpr T normalise(const T value, const T start, const T end) noexcept
	{
		assert(start != end);
		const auto width = end - start;			  // 
		const auto offsetValue = value - start;   // value relative to 0
		auto val_over_width = offsetValue / width;
		if constexpr (std::floating_point<T>)
			val_over_width = std::floor(val_over_width);
		return (offsetValue - (val_over_width * width)) + start;
	}

	template<std::floating_point T>
	constexpr T remap(T value, T start, T end, T new_start, T new_end) noexcept
	{
		assert(start != end);
		return new_start + (new_end - new_start) * ((value - start) / (end - start));
	}

	template<typename T>
	constexpr T clamp(T value, T min, T max) noexcept
	{
		return std::clamp(value, std::min(min, max), std::max(min, max));
	}

	template<typename T>
	bool is_within(T value, T min, T max)
	{
		return value >= std::min(min, max) && value < std::max(min, max);
	}

	template<typename T>
	bool range_within(T first_min, T first_max, T second_min, T second_max)
	{
		return std::max(first_min, first_max) >= std::min(second_min, second_max)
			&& std::min(first_min, first_max) <= std::max(second_min, second_max);
	}
}