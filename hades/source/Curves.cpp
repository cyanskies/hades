#include "Hades/Curves.hpp"

namespace hades
{
	template<>
	types::string lerp(types::string first, types::string second, float alpha)
	{
		assert(false && "tried to call lerp on a string");
		LOGERROR("Called lerp with a string: don't store strings in Linear Curves");
		return first;
	}
}