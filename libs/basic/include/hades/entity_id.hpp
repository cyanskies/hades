#ifndef HADES_ENTITY_ID_HPP
#define HADES_ENTITY_ID_HPP

#include "hades/strong_typedef.hpp"
#include "hades/types.hpp"

namespace hades
{
	//entity identification type
	struct entity_id_t {};
	using entity_id = strong_typedef<entity_id_t, uint16>;

	//constant value for a bad entity id
	constexpr auto bad_entity = entity_id{ std::numeric_limits<entity_id::value_type>::min() };

	template<>
	inline entity_id from_string<entity_id>(std::string_view s)
	{
		return strong_typedef_from_string<entity_id>(s);
	}
}

#endif //HADES_ENTITY_ID_HPP
