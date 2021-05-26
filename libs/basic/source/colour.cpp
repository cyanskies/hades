#include "hades/colour.hpp"

#include <string_view>

#include "hades/string.hpp"

namespace hades
{
	template<>
	colour from_string<colour>(std::string_view s)
	{
		const auto v = vector_from_string<std::vector<uint8>>(s);
		if (size(v) < 3)
			throw bad_conversion{ "a colour must have at least red, green and blue components." };

		if (size(v) > 3) //alpha is also present
			return { v[0], v[1], v[2], v[3] };
		else // alpha defaults to uint8 max
			return { v[0], v[1], v[2] };
	}

	string to_string(colour c)
	{
		return "[r:" + to_string(c.r) +
			", g:" + to_string(c.g) +
			", b:" + to_string(c.b) +
			(c.a != std::numeric_limits<colour::value_type>::max() ? (", a:" + to_string(c.a)) : "") + 
			"]";
	}
}
