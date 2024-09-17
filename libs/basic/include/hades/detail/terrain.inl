#include "hades/math.hpp"

namespace hades
{
	template<typename  Iter1, typename Iter2>
	resources::transition_tile_type get_transition_type(tile_corners corners, Iter1 begin, Iter2 end) noexcept
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

	namespace detail
	{
		template<typename Map>
			requires std::same_as<std::decay_t<Map>, terrain_map>
		auto get_triangle_height(const tile_index_t i, Map& m) noexcept
		{
			assert(std::cmp_greater_equal(m.heightmap.size(), (integer_cast<std::ptrdiff_t>(i) * 6) + 6));
			auto beg = std::next(std::begin(m.heightmap), integer_cast<std::ptrdiff_t>(i) * 6);
			using value_type = std::conditional_t<std::is_const_v<Map>, const std::uint8_t, std::uint8_t>;
			return std::span<value_type, 6>{ beg, 6 };
		}
	}

	constexpr rect_corners triangle_index_to_quad_index(const std::uint8_t i, const bool tri_type) noexcept
	{
		constexpr auto uphill = std::array<std::uint8_t, 6>{
			/*0, 1, 2, 3, 4, 5*/
			  0, 3, 1, 1, 3, 2
		};
		constexpr auto downhill = std::array<std::uint8_t, 6>{
			/*0, 1, 2, 3, 4, 5*/
			  0, 3, 2, 0, 2, 1
		};

		assert(i < 6);

		if (tri_type == terrain_map::triangle_uphill)
			return rect_corners{ uphill[i] };
		else
			return rect_corners{ downhill[i] };
	}

	constexpr std::pair<std::uint8_t, std::uint8_t> quad_index_to_triangle_index(const rect_corners c, const bool tri_type) noexcept
	{
		constexpr auto uphill = std::array<std::pair<std::uint8_t, std::uint8_t>, 4>{
			std::pair{ std::uint8_t{ 0 }, bad_triangle_index },	// top-left
			std::pair{ std::uint8_t{ 2 }, std::uint8_t{ 3 } },	// top-right
			std::pair{ std::uint8_t{ 5 }, bad_triangle_index },	// bottom-right
			std::pair{ std::uint8_t{ 1 }, std::uint8_t{ 4 } },	// bottom-left
		};
		constexpr auto downhill = std::array<std::pair<std::uint8_t, std::uint8_t>, 4>{
			std::pair{ std::uint8_t{ 0 }, std::uint8_t{ 3 } },	// top-left
			std::pair{ std::uint8_t{ 5 }, bad_triangle_index },	// top-right
			std::pair{ std::uint8_t{ 2 }, std::uint8_t{ 4 } },	// bottom-right
			std::pair{ std::uint8_t{ 1 }, bad_triangle_index },	// bottom-left
		};

		assert(c < rect_corners::last);

		if (tri_type == terrain_map::triangle_uphill)
			return uphill[enum_type(c)];
		else
			return downhill[enum_type(c)];
	}

	constexpr std::array<std::uint8_t, 2> get_height_for_top_edge(const triangle_height_data& tris) noexcept
	{
		if (tris.triangle_type == terrain_map::triangle_uphill)
		{
			return {
				tris.height[0],
				tris.height[2]
			};
		}
		else
		{
			return {
				tris.height[3],
				tris.height[5]
			};
		}
	}

	constexpr std::array<std::uint8_t, 2> get_height_for_left_edge(const triangle_height_data& tris) noexcept
	{
		return {
			tris.height[0],
			tris.height[1]
		};
	}

	constexpr std::array<std::uint8_t, 2> get_height_for_right_edge(const triangle_height_data& tris) noexcept
	{
		if (tris.triangle_type == terrain_map::triangle_uphill)
		{
			return {
				tris.height[3],
				tris.height[5]
			};
		}
		else
		{
			return {
				tris.height[5],
				tris.height[4]
			};
		}
	}

	constexpr std::array<std::uint8_t, 2> get_height_for_bottom_edge(const triangle_height_data& tris) noexcept
	{
		if (tris.triangle_type == terrain_map::triangle_uphill)
		{
			return {
				tris.height[4],
				tris.height[5]
			};
		}
		else
		{
			return {
				tris.height[1],
				tris.height[2]
			};
		}
	}

	constexpr std::array<std::uint8_t, 4> get_height_for_diag_edge(const triangle_height_data& tris) noexcept
	{
		if (tris.triangle_type == terrain_map::triangle_uphill)
		{
			return {
				tris.height[1],
				tris.height[2],
				tris.height[4],
				tris.height[3]
			};
		}
		else
		{
			return {
				tris.height[0],
				tris.height[2],
				tris.height[3],
				tris.height[4]
			};
		}
	}

	template<std::invocable<tile_position> Func>
	void for_each_adjacent_tile(const terrain_vertex_position p, const terrain_map& map, Func&& f) noexcept
	{
		const auto start = p - terrain_vertex_position{ 1, 1 };
		for_each_position_rect(start, tile_position{ 1, 1 }, get_size(map), std::forward<Func>(f));
		return;
	}

	template<std::invocable<tile_position> Func>
	void for_each_safe_adjacent_tile(terrain_vertex_position p, const terrain_map& map, Func&& f) noexcept
	{
		const auto start = p - terrain_vertex_position{ 1, 1 };
		for_each_safe_position_rect(start, tile_position{ 1, 1 }, get_size(map), std::forward<Func>(f));
		return;
	}

	template<std::invocable<tile_position, rect_corners> Func>
	static void for_each_safe_adjacent_corner(const terrain_map& m, const terrain_vertex_position p, Func&& f)
		noexcept(std::is_nothrow_invocable_v<Func, tile_position, rect_corners>)
	{
		constexpr auto corners = std::array{
			rect_corners::top_left,
			rect_corners::top_right,
			rect_corners::bottom_right,
			rect_corners::bottom_left
		};

		const auto positions = std::array{
			p,
			p - tile_position{ 1, 0 },
			p - tile_position{ 1, 1 },
			p - tile_position{ 0, 1 }
		};

		const auto world_size = get_size(m);
		constexpr auto size = std::size(corners);

		for (auto i = std::uint8_t{}; i != size; ++i)
		{
			if (within_map(world_size, positions[i]))
				std::invoke(f, positions[i], corners[i]);
		}

		return;
	}

	namespace detail
	{
		template<invocable_r<std::uint8_t, std::uint8_t> Func>
		void change_terrain_height_impl(terrain_map& m, tile_position p, rect_corners c, Func& f)
		{
			const auto index = to_tile_index(p, m);
			auto height = get_triangle_height(index, m);

			const auto& t_info = get_cliff_info(p, m);
			const auto [first, second] = quad_index_to_triangle_index(c, t_info.triangle_type);
			height[first] = std::invoke(f, height[first]);

			if (second != bad_triangle_index)
				height[second] = std::invoke(f, height[second]);

			return;
		}
	}

	template<invocable_r<std::uint8_t, std::uint8_t> Func>
	void change_terrain_height(terrain_map& m, tile_position p, rect_corners c, Func&& f)
	{
		// call each of the tiles adjacent to this corner to perform the height change
		const auto size = get_size(m);

		constexpr auto corners = std::array{
				rect_corners::bottom_right,
				rect_corners::bottom_left,
				rect_corners::top_left,
				rect_corners::top_right
		};

		auto check_tiles = [&](const std::array<tile_position, 4> &tiles) {
			for (auto i = std::uint8_t{}; i < std::size(tiles); ++i)
			{
				const auto pos = p + tiles[i];
				if (within_world(pos, size))
					detail::change_terrain_height_impl(m, pos, corners[i], f);
			}
			return;
		};
		
		switch (c)
		{
		case rect_corners::top_left:
		{
			constexpr auto tiles = std::array{
				tile_position{ -1, -1 },
				tile_position{ 0, -1 },
				tile_position{ 0, 0 },
				tile_position{ -1, 0 },
			};

			check_tiles(tiles);
			return;
		}
		case rect_corners::top_right:
		{
			constexpr auto tiles = std::array{
				tile_position{ 0, -1 },
				tile_position{ 1, -1 },
				tile_position{ 1, 0 },
				tile_position{ 0, 0 },
			};

			check_tiles(tiles);
			return;
		}
		case rect_corners::bottom_right:
		{
			constexpr auto tiles = std::array{
				tile_position{ 0, 0 },
				tile_position{ 1, 0 },
				tile_position{ 1, 1 },
				tile_position{ 0, 1 },
			};

			check_tiles(tiles);
			return;
		}
		case rect_corners::bottom_left:
		{
			constexpr auto tiles = std::array{
				tile_position{ -1, 0 },
				tile_position{ 0, 0 },
				tile_position{ 0, 1 },
				tile_position{ -1, 1 },
			};

			check_tiles(tiles);
			return;
		}
		}

		throw terrain_error{ "Called change terrain height with an invalid corner" };
	}

	template<invocable_r<std::uint8_t, std::uint8_t> Func>
	void change_terrain_height(terrain_map& m, terrain_vertex_position v, Func&& f)
	{
		// find a tile and corner and pass along to the next func
		const auto size = get_size(m);

		if (v.x == 0 && v.y == size.y) //bottom left
			change_terrain_height(m, v - tile_position{ 0, 1 }, rect_corners::bottom_left, std::forward<Func>(f));
		else if (v.x == size.x && v.y == 0) //top right
			change_terrain_height(m, v - tile_position{ 1, 0 }, rect_corners::top_right, std::forward<Func>(f));
		else if (v.x == 0 || v.y == 0) // top and left edges
			change_terrain_height(m, v, rect_corners::top_left, std::forward<Func>(f));
		else // rest of map
			change_terrain_height(m, v - tile_position{ 1, 1 }, rect_corners::bottom_right, std::forward<Func>(f));
		return;
	}
}
