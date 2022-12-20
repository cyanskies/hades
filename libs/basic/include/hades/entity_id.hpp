#ifndef HADES_ENTITY_ID_HPP
#define HADES_ENTITY_ID_HPP

#include <string_view>

#include "hades/string.hpp"
#include "hades/strong_typedef.hpp"
#include "hades/types.hpp"
#include "hades/utility.hpp"

namespace hades
{
	//entity identification type
	struct entity_id_t {};
	// TODO: upgrade this to uint32
	//		when storing id's in actions we need to convert them to signed 32's
	//		using the to_int32 and from_int32 functions below
	using entity_id = strong_typedef<entity_id_t, uint32>;

	//constant value for a bad entity id
	constexpr auto bad_entity = entity_id{ std::numeric_limits<typename entity_id::value_type>::min() };

	template<>
	inline entity_id from_string<entity_id>(std::string_view s)
	{
		return strong_typedef_from_string<entity_id>(s);
	}

	// encodes an entity_id as an int32
	constexpr int32 to_int32(const entity_id e)
	{
		constexpr auto int_max = std::numeric_limits<int32>::max();
		const auto val = to_value(e);
		if (val > static_cast<uint32>(int_max))
			return static_cast<int32>(val - int_max);
		else
			return static_cast<int32>(val) - int_max;
	}

	// pass id encoded as an int32 to convert back into entity id
	constexpr entity_id to_entity_id(const int32 i)
	{
		constexpr auto int_max = std::numeric_limits<int32>::max();
		if (i < 0)
			return entity_id{ static_cast<uint32>(i + int_max) };
		else
			return entity_id{ static_cast<uint32>(i) + int_max };
	}
}

#endif //HADES_ENTITY_ID_HPP
