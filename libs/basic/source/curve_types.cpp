#include "hades/curve_types.hpp"

namespace hades
{
	template<>
	object_ref from_string<object_ref>(std::string_view s)
	{
		return { from_string<entity_id>(s) };
	}
}
