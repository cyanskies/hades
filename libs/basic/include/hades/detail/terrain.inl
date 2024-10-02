#include <tuple>

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
		// Returns an editable span into a tiles height
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

	// Converts base 6 triangle index's into quad corner indexes
	// Triangle indicies are defined in struct terrain_map (terrain.hpp)
	// Quad corners are defined in enum rect_corners (rectangle_math.hpp)
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
			std::pair{ bad_triangle_index, std::uint8_t{ 5 } },	// bottom-right
			std::pair{ std::uint8_t{ 1 }, std::uint8_t{ 4 } },	// bottom-left
		};
		constexpr auto downhill = std::array<std::pair<std::uint8_t, std::uint8_t>, 4>{
			std::pair{ std::uint8_t{ 0 }, std::uint8_t{ 3 } },	// top-left
			std::pair{ bad_triangle_index, std::uint8_t{ 5 } },	// top-right
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
		void change_terrain_height_bad_impl(terrain_map& m, tile_position p, rect_corners c, const std::uint8_t height_min, const std::uint8_t height_max, Func& f)
		{
			const auto index = to_tile_index(p, m);
			auto height = get_triangle_height(index, m);

			const auto& t_info = get_cliff_info(p, m);
			const auto [first, second] = quad_index_to_triangle_index(c, t_info.triangle_type);
			if(first != bad_triangle_index)
				height[first] = std::clamp(std::invoke(f, height[first]), height_min, height_max);
			if (second != bad_triangle_index)
				height[second] = std::clamp(std::invoke(f, height[second]), height_min, height_max);

			return;
		}
	}

	template<invocable_r<std::uint8_t, std::uint8_t> Func>
	void change_terrain_height_bad(terrain_map& m, tile_position p, rect_corners c,
		const std::uint8_t height_min, const std::uint8_t height_max, Func&& f)
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
					detail::change_terrain_height_bad_impl(m, pos, corners[i], height_min, height_max, f);
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
	void change_terrain_height_bad(terrain_map& m, terrain_vertex_position v, const resources::terrain_settings& settings, Func&& f)
	{
		// find a tile and corner and pass along to the next func
		const auto size = get_size(m);
		const auto h_min = settings.height_min;
		const auto h_max = settings.height_max;
		if (v.x == 0 && v.y == size.y) //bottom left
			change_terrain_height_bad(m, v - tile_position{ 0, 1 }, rect_corners::bottom_left, h_min, h_max, std::forward<Func>(f));
		else if (v.x == size.x && v.y == 0) //top right
			change_terrain_height_bad(m, v - tile_position{ 1, 0 }, rect_corners::top_right, h_min, h_max, std::forward<Func>(f));
		else if (v.x == 0 || v.y == 0) // top and left edges
			change_terrain_height_bad(m, v, rect_corners::top_left, h_min, h_max, std::forward<Func>(f));
		else // rest of map
			change_terrain_height_bad(m, v - tile_position{ 1, 1 }, rect_corners::bottom_right, h_min, h_max, std::forward<Func>(f));
		return;
	}

	namespace detail
	{
		using specific_vertex = std::tuple<tile_position, rect_corners, bool>;
		// returns next clockwise vertex, or the same vertex if none exists
		// stops if the next vertex is split by a cliff.
		// A specific vertex is the individual height value for part of a vertex

		// this is actually useless
		enum class specific_vertex_clockwise_order_enum 
		{
			top_left_tri2,		// downhill only
			begin = top_left_tri2,
			top_left,
			top_right_tri2,		// uphill only
			top_right,
			bottom_right,
			bottom_right_tri2,	// downhill only
			bottom_left,
			bottom_left_tri2,	// uphill only
			end
		};

		// TODO: possibly move this impl into terrain.cpp and same for next_specific_vertex_anti_clockwise
		inline specific_vertex next_specific_vertex_clockwise(const specific_vertex v, const terrain_map& m)
		{
			using enum rect_corners;
			const auto& [pos, corner, left_tri] = v;
			const auto cliff_info = get_cliff_info(pos, m);
			const auto uphill = cliff_info.triangle_type == terrain_map::triangle_uphill;
			constexpr auto left_triangle = true;
			constexpr auto right_triangle = false;
			switch (corner)
			{
			case top_left:
				if (left_tri)
				{
					const auto next_tile = pos - tile_position{ 1, 0 };
					const auto next_info = get_cliff_info(next_tile, m);
					if (next_info.right)
						return v;

					constexpr auto next_tri = right_triangle;
					return { next_tile, top_right, next_tri };
				}
				else
				{
					assert(!uphill); // no right triangle for this corner in uphill
					if (!cliff_info.diag_downhill)
						return { pos, corner, left_triangle };

					return v;
				}
			case top_right:
				// TODO: this if statement looks a little wiggly
				//		I feel like their is a better way to write this
				if (left_tri || !uphill)
				{
					const auto next_tile = pos - tile_position{ 0, 1 };
					const auto next_info = get_cliff_info(next_tile, m);
					if (next_info.bottom) // blocked by cliff
						return v;

					const auto next_tri = next_info.triangle_type == terrain_map::triangle_downhill;
					return { next_tile, bottom_right, next_tri };
				}
				else if (uphill) // right tri and uphill triangle
				{
					assert(!left_tri);
					if (cliff_info.diag)
						return v; // blocked by cliff

					// move to left tri
					return { pos, corner, left_triangle };
				}
				else
				{
					//assert(false); // unreachable
					throw logic_error{"unreachable"};
					return {};
				}
			case bottom_right:
				if (left_tri)
				{
					assert(!uphill);
					// move to right tri
					if (cliff_info.diag)
					{
						assert(cliff_info.diag_downhill);
						return v;
					}

					return { pos, corner, right_triangle };
				}
				else
				{
					if (cliff_info.right) // blocked by cliffs
						return v;
					const auto next_tile = pos + tile_position{ 1, 0 };
					return { next_tile, bottom_left, left_triangle };
				}
			case bottom_left:
				if (left_tri && uphill)
				{
					// move to right tri
					if (cliff_info.diag)
						return v;
					return { pos, corner, right_triangle };
				}
				else if(cliff_info.bottom)
				{
					return v;
				}
				else
				{
					const auto next_tile = pos + tile_position{ 0, 1 };
					const auto next_info = get_cliff_info(next_tile, m);
					const auto next_tri = next_info.triangle_type == terrain_map::triangle_uphill;
					return { next_tile, top_left, next_tri };
				}
			default:
				throw logic_error{ "invalid corner value passed to next_specific_vertex_clockwise" };
			}
		}

		inline specific_vertex next_specific_vertex_anti_clockwise(const specific_vertex v, const terrain_map& m)
		{
			using enum rect_corners;
			const auto& [pos, corner, left_tri] = v;
			const auto cliff_info = get_cliff_info(pos, m);
			const auto uphill = cliff_info.triangle_type == terrain_map::triangle_uphill;
			constexpr auto left_triangle = true;
			constexpr auto right_triangle = false;
			switch (corner)
			{
			case top_left:
				if (left_tri && !uphill)
				{
					if (cliff_info.diag) // blocked by cliff
						return v;
					return { pos, corner, right_triangle };
				}
				else
				{
					const auto next_tile = pos - tile_position{ 0, 1 };
					const auto next_info = get_cliff_info(next_tile, m);
					if (next_info.bottom) // blocked by cliff
						return v;

					const auto next_tri = next_info.triangle_type == terrain_map::triangle_downhill;
					return { next_tile, bottom_left, next_tri };
				}
			case bottom_left:
				if (left_tri)
				{
					// go to next tile
					const auto next_tile = pos - tile_position{ 1, 0 };
					const auto next_info = get_cliff_info(next_tile, m);
					if (next_info.right) // blocked by cliff
						return v;

					return { next_tile, bottom_right, right_triangle };
				}
				else 
				{
					// move to left tri
					assert(uphill); // there is no right_triangle for this corner in downhill
					if (cliff_info.diag) // blocked by cliff
						return v;
					return { pos, corner, left_triangle };
				}
			case bottom_right:
				// TODO: this if statement looks a little wiggly
				//		I feel like their is a better way to write this
				if (left_tri || uphill)
				{
					// go to next tile
					const auto next_tile = pos + tile_position{ 0, 1 };
					if (cliff_info.bottom) // blocked by cliff
						return v;

					const auto next_info = get_cliff_info(next_tile, m);
					const auto next_tri = next_info.triangle_type == terrain_map::triangle_uphill;
					return { next_tile, top_right, next_tri };
				}
				else if(!left_tri)
				{
					assert(!uphill); // right_tri only exists for downhill on this corner
					if (cliff_info.diag) // blocked by cliff
						return v;
					return { pos, corner, left_triangle };
				}
				else
				{
					//assert(false); // unreachable
					throw logic_error{ "unreachable" };
					return {};
				}
			case top_right:
				if (left_tri)
				{
					assert(uphill); // downhill doesn't have a right_tri on this corner
					if (cliff_info.diag) // blocked by cliff
						return v;
					return { pos, corner, right_triangle };
				}
				else 
				{
					if (cliff_info.right) // blocked by cliff
						return v;
					const auto next_tile = pos + tile_position{ 1, 0 };
					return { next_tile, top_left, left_triangle };
				}
			default:
				throw logic_error{ "invalid corner value passed to next_specific_vertex_anti_clockwise" };
			}
		}

		template<invocable_r<std::uint8_t, std::uint8_t> Func>
		void change_terrain_vertex_height_impl(terrain_map& m, const specific_vertex& vertex, const tile_position world_size, const std::uint8_t height_min, const std::uint8_t height_max, Func& f)
		{
			const auto& [p, c, t] = vertex;

			if (!within_world(p, world_size))
				return;

			const auto index = to_tile_index(p, m);
			auto height = get_triangle_height(index, m);

			const auto& t_info = get_cliff_info(p, m);
			const auto [first, second] = quad_index_to_triangle_index(c, t_info.triangle_type);
			const auto vertex_index = t ? first : second;

			height[vertex_index] = std::clamp(std::invoke(f, height[vertex_index]), height_min, height_max);
			return;
		}
	}

	template<invocable_r<std::uint8_t, std::uint8_t> Func>
	void change_terrain_height(const tile_position tile, const rect_corners corner, const bool left_triangle, terrain_map& m, const resources::terrain_settings& settings, Func&& f)
	{
		// check world limits
		const auto size = get_size(m);
		if (!within_world(tile, size))
			throw terrain_error{ "Invalid argument passed to change_terrain_height: 'tile' was outside the map" };

		const auto min_h = settings.height_min;
		const auto max_h = settings.height_max;

		const auto specific_start = detail::specific_vertex{ tile, corner, left_triangle };
		// change the central target
		detail::change_terrain_vertex_height_impl(m, specific_start, size, min_h, max_h, f);

		auto specific_next = detail::next_specific_vertex_clockwise(specific_start, m);
		auto specific_current = specific_start;

		// rotate clockwise
		while (specific_next != specific_current &&
			specific_next != specific_start)
		{
			detail::change_terrain_vertex_height_impl(m, specific_next, size, min_h, max_h, f);
			specific_current = specific_next;
			specific_next = detail::next_specific_vertex_clockwise(specific_current, m);
		}

		// rotate anti-clockwise
		if (specific_next != specific_start || // if(specific_next == start, then we made the full rotation
			// vv All the specific_* are the same, means we got blocked before making any rotation
			(specific_next == specific_start && 
			specific_next == specific_current))
		{
			specific_current = specific_start;
			specific_next = detail::next_specific_vertex_anti_clockwise(specific_current, m);
			// NOTE: We don't need to check against specific_start, we only
			//		go anti clockwise if the previous direction hit a cliff.
			//		We are guaranteed to hit a cliff at some point in this loop
			while (specific_next != specific_current)
			{
				detail::change_terrain_vertex_height_impl(m, specific_next, size, min_h, max_h, f);
				specific_current = specific_next;
				specific_next = detail::next_specific_vertex_anti_clockwise(specific_current, m);
			}
		}
		return;
	}
}
