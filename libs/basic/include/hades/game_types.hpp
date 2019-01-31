#ifndef HADES_GAME_TYPES_HPP
#define HADES_GAME_TYPES_HPP

#include <string_view>
#include <vector>

#include "hades/strong_typedef.hpp"
#include "hades/types.hpp"
#include "hades/uniqueid.hpp"
#include "hades/utility.hpp"

namespace hades
{
	//entity identification type
	struct entity_id_t {};
	using entity_id = strong_typedef<entity_id_t, int32>;

	//constant value for a bad entity id
	constexpr auto bad_entity = entity_id{std::numeric_limits<entity_id::value_type>::min()};
	 
	template<>
	inline entity_id from_string<entity_id>(std::string_view s)
	{
		return entity_id{ from_string<entity_id::value_type>(s) };
	}

	template<>
	inline string to_string<entity_id>(entity_id id)
	{
		return to_string(static_cast<entity_id::value_type>(id));
	}

	//level size type

	//world unit type
	//used for position ranges and sizes
	using world_unit = float;

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
