#ifndef HADES_GAME_TYPES_HPP
#define HADES_GAME_TYPES_HPP

#include <array>
#include <vector>

#include "hades/curve_types.hpp"
#include "hades/entity_id.hpp"
#include "hades/rectangle_math.hpp"
#include "hades/uniqueid.hpp"

namespace hades
{
	//world unit type
	//used for position ranges and sizes
	//game_unit is used to measure distance in the game world
	// game units are world pixels expressed in floats
	using world_unit_t = curve_types::float_t;
	using world_vector_t = curve_types::vec2_float;
	using world_rect_t = rect_t<world_unit_t>;

	//tag types
	//these are used for listing capabilities or effect info
	using tag_t = unique_id;
	using tag_list = std::vector<tag_t>;

	// check_tags accepts a tag list and a array of tags to check,
	// it returns an array indicating whether the tags in the check_list were found.
	template<std::size_t N>
	constexpr std::array<bool, N> check_tags(const tag_list& list, const std::array<tag_list::value_type, N>& check_list) noexcept
	{
		auto ret = std::array<bool, N>{};
		ret.fill(false);

		for (const auto elm : list)
		{
			for (auto i = std::size_t{}; i < N; ++i)
			{
				if (elm == check_list[i])
				{
					ret[i] = true;
					break;
				}
			}
		}

		return ret;
	}
}

#endif //!HADES_GAME_TYPES_HPP
