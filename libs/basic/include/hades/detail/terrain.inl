#include "hades/math.hpp"

namespace hades
{
	template<typename  Iter1, typename Iter2>
	resources::transition_tile_type get_transition_type(tile_corners corners, Iter1 begin, Iter2 end)
	{
		//transitions are selected between any terrain and the empty terrain
		const auto has_terrain = [begin, end](const resources::terrain *t)->bool {
			return std::any_of(begin, end, [t](const resources::terrain* t2) {
				return t == t2;// return true if this terrain is in the beg/end range
				});
		};

		//if the corner contains any of the listed terrains
		// then it is true, otherwise false
		const auto terrain_corners = std::array{
			has_terrain(get_corner(corners, rect_corners::top_left)),
			has_terrain(get_corner(corners, rect_corners::top_right)),
			has_terrain(get_corner(corners, rect_corners::bottom_right)),
			has_terrain(get_corner(corners, rect_corners::bottom_left))
		};

		return get_transition_type(terrain_corners);
	}
}