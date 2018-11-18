#ifndef HADES_GAME_TYPES_HPP
#define HADES_GAME_TYPES_HPP

#include "hades/strong_typedef.hpp"
#include "hades/types.hpp"
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

	//something else?
}

#endif //!HADES_GAME_TYPES_HPP
