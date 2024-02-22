#include "hades/math.hpp"

namespace hades
{
	template<typename  Iter1, typename Iter2>
	resources::transition_tile_type get_transition_type(tile_corners corners, Iter1 begin, Iter2 end)
	{
		//transitions are selected between any terrain and the empty terrain
		const auto has_terrain = [begin, end](const resources::terrain *t)->bool {
			return std::any_of(begin, end, [t](resources::resource_link<resources::terrain> t2) {
				return t == t2.get();// return true if this terrain is in the beg/end range
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

	constexpr std::size_t triangle_index_to_quad_index(const std::size_t i, const bool tri_type) noexcept
	{
		constexpr auto uphill = std::array<std::size_t, 6>{
			0, 1, 3, 1, 2, 3
		};
		constexpr auto downhill = std::array<std::size_t, 6>{
			0, 2, 3, 0, 1, 2
		};

		if (tri_type == terrain_map::triangle_uphill)
			return uphill[i];
		else
			return downhill[i];
	}

	template<std::ranges::random_access_range Range,
		std::invocable<typename Range::value_type, typename Range::value_type> Func>
		requires requires(const Range& r, Func&& f, std::size_t i, typename Range::value_type v)
	{
		r[i];
		{ f(v, v) } -> std::same_as<typename Range::value_type>;
	}
	typename Range::value_type access_triangles_as_quad(const Range& r, const bool tri_type, const std::size_t i, Func&& f)
	{
		if (tri_type == terrain_map::triangle_uphill)
		{
			switch (i)
			{
			case 0:
				return r[0];
			case 1:
				return std::invoke(f, r[1], r[3]);
			case 2:
				return r[4];
			case 3:
				return std::invoke(f, r[2], r[5]);
			}
		}
		else
		{
			switch (i)
			{
			case 0:
				return std::invoke(f, r[0], r[3]);
			case 1:
				return r[4];
			case 2:
				return std::invoke(f, r[1], r[5]);
			case 3:
				return r[2];
			}
		}

		throw invalid_argument{ "index out of range for access_triangles_as_quad" };
	}
}