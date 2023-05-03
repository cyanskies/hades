#include "hades/math.hpp"

#include <algorithm>
#include <numbers>

namespace hades
{
	template<typename T>
	constexpr auto to_radians(T degrees) noexcept
	{
		return (degrees * std::numbers::pi_v<std::conditional_t<std::floating_point<T>, T, float>>) / 180;
	}

	template<std::floating_point Float>
	constexpr auto to_degrees(Float radians) noexcept
	{
		auto degs = (radians * 180.f) / std::numbers::pi_v<Float>;
		if (degs < 0)
			return degs + 360;
		return degs;
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