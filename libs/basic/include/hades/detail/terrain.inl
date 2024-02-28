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

	namespace detail
	{
		template<typename Map>
			requires std::same_as<std::decay_t<Map>, terrain_map>
		auto get_triangle_height(const tile_index_t i, Map& m)
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
			0, 1, 3, 1, 2, 3
		};
		constexpr auto downhill = std::array<std::uint8_t, 6>{
			0, 2, 3, 0, 1, 2
		};

		if (tri_type == terrain_map::triangle_uphill)
			return rect_corners{ uphill[i] };
		else
			return rect_corners{ downhill[i] };
	}

	constexpr std::pair<std::uint8_t, std::uint8_t> quad_index_to_triangle_index(const rect_corners c, const bool tri_type) noexcept
	{
		constexpr auto uphill = std::array<std::pair<std::uint8_t, std::uint8_t>, 4>{
			std::pair{ std::uint8_t{ 0 }, bad_triangle_index },	// top-left
			std::pair{ std::uint8_t{ 1 }, std::uint8_t{ 3 } },	// top-right
			std::pair{ std::uint8_t{ 4 }, bad_triangle_index },	// bottom-right
			std::pair{ std::uint8_t{ 2 }, std::uint8_t{ 5} },	// bottom-left
		};
		constexpr auto downhill = std::array<std::pair<std::uint8_t, std::uint8_t>, 4>{
			std::pair{ std::uint8_t{ 0 }, std::uint8_t{ 3 } },	// top-left
			std::pair{ std::uint8_t{ 4 }, bad_triangle_index },	// top-right
			std::pair{ std::uint8_t{ 1 }, std::uint8_t{ 5 } },	// bottom-right
			std::pair{ std::uint8_t{ 2 }, bad_triangle_index },	// bottom-left
		};

		if (tri_type == terrain_map::triangle_uphill)
			return uphill[enum_type(c)];
		else
			return downhill[enum_type(c)];
	}

	template<std::ranges::random_access_range Range,
		std::invocable<typename Range::value_type, typename Range::value_type> Func>
		requires requires(const Range& r, Func&& f, std::size_t i, typename Range::value_type v)
	{
		r[i];
		{ f(v, v) } -> std::same_as<typename Range::value_type>;
	}
	typename Range::value_type read_triangles_as_quad(const Range& r, const bool tri_type, const std::size_t i, Func&& f)
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

		throw invalid_argument{ "index out of range for read_triangles_as_quad" };
	}

	template<std::ranges::random_access_range Range,
		std::invocable<typename Range::value_type&> Func>
		requires requires(Range& r, Func&& f, std::size_t i, typename Range::value_type v)
	{
		{ r[i] } -> std::same_as<std::remove_const_t<typename Range::value_type&>>;
	}
	void invoke_on_triangle_corner(Range& r, bool tri_type, rect_corners c, Func&& f)
	{
		const auto i = enum_type(c);
		if (tri_type == terrain_map::triangle_uphill)
		{
			switch (i)
			{
			case 0:
				std::invoke(f, r[0]);
				return;
			case 1:
				std::invoke(f, r[1]);
				std::invoke(f, r[3]);
				return;
			case 2:
				std::invoke(f, r[4]);
				return;
			case 3:
				std::invoke(f, r[2]); 
				std::invoke(f, r[5]);
				return;
			}
		}
		else
		{
			switch (i)
			{
			case 0:
				std::invoke(f, r[0]);
				std::invoke(f, r[3]);
				return;
			case 1:
				std::invoke(f, r[4]);
				return;
			case 2:
				std::invoke(f, r[1]);
				std::invoke(f, r[5]);
				return;
			case 3:
				std::invoke(f, r[2]);
				return;
			}
		}

		throw invalid_argument{ "index out of range for invoke_on_triangle_corner" };
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

	template<std::invocable<triangle_info> Func>
	void for_each_adjacent_triangle(terrain_vertex_position v, const terrain_map& m, Func&& f) noexcept
	{ 
		const auto size = get_size(m);

		if (v.x == 0 && v.y == size.y) //bottom left
			for_each_adjacent_triangle(v - tile_position{ 0, 1 }, rect_corners::bottom_left, m, std::forward<Func>(f));
		else if (v.x == size.x && v.y == 0) //top right
			for_each_adjacent_triangle(v - tile_position{ 1, 0 }, rect_corners::top_right, m, std::forward<Func>(f));
		else if (v.x == 0 || v.y == 0) // top and left edges
			for_each_adjacent_triangle(v, rect_corners::top_left, m, std::forward<Func>(f));
		else // rest of map
			for_each_adjacent_triangle(v - tile_position{ 1, 1 }, rect_corners::bottom_right, m, std::forward<Func>(f));
		return;
	}

	namespace detail
	{
		// perform for_each adjacent 
		template<std::invocable<triangle_info> Func>
		void for_each_adjacent_triangle(triangle_info t, const terrain_map& m, Func&& f) noexcept
		{

		}

		// perform for_each_adjacent tri only on this tile
		template<std::invocable<triangle_info> Func>
		void for_each_adjacent_triangle_impl(tile_position p, rect_corners c, const terrain_map& m, Func& f) noexcept
		{
			const auto& t_info = get_cliff_info(p, m);
			const auto uphill = t_info.triangle_type == terrain_map::triangle_uphill;

			// there is a very apparent pattern here, possible to reduce this code?
			switch (c)
			{
			case rect_corners::top_left:
			{
				if (uphill)
					std::invoke(f, triangle_info{ p, c, terrain_map::triangle_left });
				else
				{
					std::invoke(f, triangle_info{ p, c, terrain_map::triangle_left });
					std::invoke(f, triangle_info{ p, c, terrain_map::triangle_right });
				}
			}
			case rect_corners::top_right:
			{
				if (uphill)
				{
					std::invoke(f, triangle_info{ p, c, terrain_map::triangle_left });
					std::invoke(f, triangle_info{ p, c, terrain_map::triangle_right });
				}
				else
				{
					std::invoke(f, triangle_info{ p, c, terrain_map::triangle_right });
				}
			}
			case rect_corners::bottom_right:
			{
				if (uphill)
					std::invoke(f, triangle_info{ p, c, terrain_map::triangle_right });
				else
				{
					std::invoke(f, triangle_info{ p, c, terrain_map::triangle_left });
					std::invoke(f, triangle_info{ p, c, terrain_map::triangle_right });
				}
			}
			case rect_corners::bottom_left:
			{
				if (uphill)
				{
					std::invoke(f, triangle_info{ p, c, terrain_map::triangle_left });
					std::invoke(f, triangle_info{ p, c, terrain_map::triangle_right });
				}
				else
				{
					std::invoke(f, triangle_info{ p, c, terrain_map::triangle_left });
				}
			}
			}
		}
	}

	template<std::invocable<triangle_info> Func>
	void for_each_adjacent_triangle(tile_position p, rect_corners c, const terrain_map&, Func&& f) noexcept
	{
		// 

	}

	template<std::invocable<triangle_info> Func>
	void for_each_adjacent_triangle(triangle_info t, const terrain_map& m, Func&& f) noexcept
	{

	}

	namespace detail
	{
		template<std::invocable<std::uint8_t> Func>
			requires std::same_as<std::invoke_result_t<Func, std::uint8_t>, std::uint8_t>
		void change_terrain_height_corner_impl(terrain_map& m, tile_position p, rect_corners c, std::int16_t amount, Func f)
		{
		}

		template<std::invocable<std::uint8_t> Func>
			requires std::same_as<std::invoke_result_t<Func, std::uint8_t>, std::uint8_t>
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

	template<std::invocable<std::uint8_t> Func>
		requires std::same_as<std::invoke_result_t<Func, std::uint8_t>, std::uint8_t>
	void change_terrain_height2(terrain_map& m, tile_position p, rect_corners c, Func&& f)
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

	template<std::invocable<std::uint8_t> Func>
		requires std::same_as<std::invoke_result_t<Func, std::uint8_t>, std::uint8_t>
	void change_terrain_height2(terrain_map& m, terrain_vertex_position v, Func&& f)
	{
		// find a tile and corner and pass along to the next func
		const auto size = get_size(m);

		if (v.x == 0 && v.y == size.y) //bottom left
			change_terrain_height2(m, v - tile_position{ 0, 1 }, rect_corners::bottom_left, std::forward<Func>(f));
		else if (v.x == size.x && v.y == 0) //top right
			change_terrain_height2(m, v - tile_position{ 1, 0 }, rect_corners::top_right, std::forward<Func>(f));
		else if (v.x == 0 || v.y == 0) // top and left edges
			change_terrain_height2(m, v, rect_corners::top_left, std::forward<Func>(f));
		else // rest of map
			change_terrain_height2(m, v - tile_position{ 1, 1 }, rect_corners::bottom_right, std::forward<Func>(f));
		return;
	}
}
