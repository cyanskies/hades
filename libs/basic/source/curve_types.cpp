#include "hades/curve_types.hpp"

namespace hades
{
	// name for bad object ref in save files and modding files
	constexpr auto bad_object_string = "null-obj";

	template<>
	object_ref from_string<object_ref>(std::string_view s)
	{
		if (s == bad_object_string)
			return {};

		return { from_string<entity_id>(s) };
	}

	string to_string(object_ref o)
	{
		if (o == object_ref{})
			return bad_object_string;

		return to_string(o.id);
	}
}
