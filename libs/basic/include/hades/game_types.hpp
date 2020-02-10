#ifndef HADES_GAME_TYPES_HPP
#define HADES_GAME_TYPES_HPP

#include <string_view>
#include <vector>

#include "hades/curve_extra.hpp"
#include "hades/entity_id.hpp"
#include "hades/rectangle_math.hpp"
#include "hades/strong_typedef.hpp"
#include "hades/types.hpp"
#include "hades/uniqueid.hpp"
#include "hades/utility.hpp"
#include "hades/vector_math.hpp"

namespace hades
{
	//world unit type
	//used for position ranges and sizes
	//game_unit is used to measure distance in the game world
	// game units are world pixels expressed in floats
	using world_unit_t = resources::curve_types::float_t;
	using world_vector_t = resources::curve_types::vec2_float;
	using world_rect_t = rect_t<world_unit_t>;

	//tag types
	//these are used for listing capabilities or effect info
	using tag_t = unique_id;
	using tag_list = std::vector<tag_t>;

	//uses the unique_id overloads of to_str and from_str

	//engines support types

	//limit is 65535, well in excess of current hardware capabilities
	//  max texture size for older hardware is 512
	//  max size for modern hardware is 8192 or higher
	using texture_size_t = uint16;
}

#endif //!HADES_GAME_TYPES_HPP
