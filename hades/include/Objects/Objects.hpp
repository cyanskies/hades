#ifndef OBJECTS_OBJECTS_HPP
#define OBJECTS_OBJECTS_HPP

#include <vector>

#include "Hades/simple_resources.hpp"
#include "Hades/Types.hpp"

namespace objects
{
	struct object_info
	{
		hades::data::UniqueId id;
		using curve_pair = std::tuple<hades::data::UniqueId, hades::resources::curve_default_value>;
		std::vector<curve_pair> curves;
	};

	using level_size_t = hades::types::uint32;

	//A level is laid out in the same way as a save file
	// but doesn't store the full curve history
	struct level
	{
		//map size in pixels
		level_size_t map_x = 0, map_y = 0;
		//sequence of object
		    //list of curve values, not including defaults
		std::vector<object_info> objects;
	};
}

#endif // !OBJECTS_OBJECTS_HPP
