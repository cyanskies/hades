#ifndef HADES_GAME_TYPES_HPP
#define HADES_GAME_TYPES_HPP

#include <array>
#include <numeric>
#include <vector>

// Toggle building a list of uniques as strings for viewing in the debugger
#define HADES_DISPLAY_UNIQUE_NAMES 0

#include "hades/curve_types.hpp"

#if HADES_DISPLAY_UNIQUE_NAMES
#include "hades/data.hpp"
#endif

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
	constexpr std::array<bool, N> check_tags(const tag_list& list, std::array<tag_list::value_type, N> check_list) noexcept
	{
		#if HADES_DISPLAY_UNIQUE_NAMES
		auto check_tags = std::array<std::string_view, N>{};
		std::ranges::transform(check_list, std::begin(check_tags), data::get_as_string);
		
		auto list_tags = std::vector<std::string_view>{};
		list_tags.reserve(size(list));
		std::ranges::transform(list, std::back_inserter(list_tags), data::get_as_string);
		#endif

		auto ret = std::array<bool, N>{};
		ret.fill(false);

		const auto size_check = size(check_list);
		for (auto i = std::size_t{}; i < size_check; ++i)
		{
			if (std::ranges::any_of(list, [val = check_list[i]](auto& id) {
				return id == val;
				}))
			{
				ret[i] = true;
			}
		}

		return ret;
	}
}

#endif //!HADES_GAME_TYPES_HPP
