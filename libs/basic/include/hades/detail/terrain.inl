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
		// Undefined behaviour if 'i' is invalid
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

	[[nodiscard]]
	constexpr terrain_vertex_position to_vertex_position(const tile_position p, const rect_corners c) noexcept
	{
		using enum rect_corners;
		switch (c)
		{
		default:
			[[fallthrough]];
		case top_left:
			return p;
		case top_right:
			return p + tile_position{ 1, 0 };
		case bottom_left:
			return p + tile_position{ 0, 1 };
		case bottom_right:
			return p + tile_position{ 1, 1 };
		}
	}

	// NOTE: specific_vertex defined here so to_vertex_position can use it
	namespace detail
	{
		// Targets a vertex in a specific triangle
		using specific_vertex = std::tuple<tile_position, rect_corners, bool>;
		constexpr auto bad_specific_vertex = specific_vertex{ bad_tile_position, rect_corners::last, {} };
	}

	[[nodiscard]]
	constexpr terrain_vertex_position to_vertex_position(detail::specific_vertex sv) noexcept
	{
		const auto pos = std::get<0>(sv);
		const auto corner = std::get<1>(sv);
		return to_vertex_position(pos, corner);
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
		// the corner in each of the below tiles that points to our vertex
		constexpr auto corners = std::array{
			rect_corners::top_left,
			rect_corners::top_right,
			rect_corners::bottom_right,
			rect_corners::bottom_left
		};

		// each tile adjacent to this vertex
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
		// NOTE: speific_vertex is defined higher in the file
		[[nodiscard]]
		inline constexpr bool is_bad(const specific_vertex& v) noexcept
		{
			return std::get<0>(v) == bad_tile_position;
		}

		// returns next clockwise vertex
		// if BlockOnCliff is true, then returns the parameter if the next
		// vertex is on the other side of a cliff.
		// A specific vertex is the individual height value for part of a vertex
		template<bool BlockOnCliff = true>
		inline specific_vertex next_specific_vertex_clockwise(const specific_vertex v, const tile_position world_size, const terrain_map& m)
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
					if (next_tile.x < 0)
						return bad_specific_vertex;

					if constexpr (BlockOnCliff)
					{
						const auto next_info = get_cliff_info(next_tile, m);
						if (next_info.right)
							return bad_specific_vertex;
					}

					return { next_tile, top_right, right_triangle };
				}
				else
				{
					assert(!uphill); // no right triangle for this corner in uphill
					if constexpr (BlockOnCliff)
					{
						if (cliff_info.diag_downhill)
							return bad_specific_vertex;
					}

					return { pos, corner, left_triangle };
				}
			case top_right:
				// TODO: this if statement looks a little wiggly
				//		I feel like their is a better way to write this
				if (left_tri || !uphill)
				{
					const auto next_tile = pos - tile_position{ 0, 1 };
					if (next_tile.y < 0)
						return bad_specific_vertex;

					const auto next_info = get_cliff_info(next_tile, m);
					if constexpr (BlockOnCliff)
					{
						if (next_info.bottom) // blocked by cliff
							return bad_specific_vertex;
					}

					const auto next_tri = next_info.triangle_type == terrain_map::triangle_downhill;
					return { next_tile, bottom_right, next_tri };
				}
				else if (uphill) // right tri and uphill triangle
				{
					assert(!left_tri);
					if constexpr (BlockOnCliff)
					{
						if (cliff_info.diag)
							return bad_specific_vertex; // blocked by cliff
					}

					// move to left tri
					return { pos, corner, left_triangle };
				}
				else
				{
					//assert(false); // unreachable
					throw logic_error{ "unreachable" };
				}
			case bottom_right:
				if (left_tri)
				{
					assert(!uphill);
					// move to right tri
					if constexpr (BlockOnCliff)
					{
						if (cliff_info.diag)
						{
							assert(cliff_info.diag_downhill);
							return bad_specific_vertex;
						}
					}

					return { pos, corner, right_triangle };
				}
				else
				{
					if constexpr (BlockOnCliff)
					{
						if (cliff_info.right) // blocked by cliffs
							return bad_specific_vertex;
					}
					const auto next_tile = pos + tile_position{ 1, 0 };
					if (next_tile.x >= world_size.x)
						return bad_specific_vertex;

					return { next_tile, bottom_left, left_triangle };
				}
			case bottom_left:
				if (left_tri && uphill)
				{
					// move to right tri
					if constexpr (BlockOnCliff)
					{
						if (cliff_info.diag)
							return bad_specific_vertex;
					}
					return { pos, corner, right_triangle };
				}
				else
				{
					if constexpr (BlockOnCliff)
					{
						if (cliff_info.bottom)
							return v;
					}

					const auto next_tile = pos + tile_position{ 0, 1 };
					if (next_tile.y >= world_size.y)
						return bad_specific_vertex;
						
					const auto next_info = get_cliff_info(next_tile, m);
					const auto next_tri = next_info.triangle_type == terrain_map::triangle_uphill;
					return { next_tile, top_left, next_tri };
				}
			default:
				throw logic_error{ "invalid corner value passed to next_specific_vertex_clockwise" };
			}
		}

		template<bool BlockOnCliff = true>
		inline specific_vertex next_specific_vertex_anti_clockwise(const specific_vertex v, const tile_position world_size, const terrain_map& m)
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
					if constexpr (BlockOnCliff)
					{
						if (cliff_info.diag) // blocked by cliff
							return bad_specific_vertex;
					}
					return { pos, corner, right_triangle };
				}
				else
				{
					const auto next_tile = pos - tile_position{ 0, 1 };
					if (next_tile.y < 0)
						return bad_specific_vertex;

					const auto next_info = get_cliff_info(next_tile, m);
					if constexpr (BlockOnCliff)
					{
						if (next_info.bottom) // blocked by cliff
							return bad_specific_vertex;
					}
					const auto next_tri = next_info.triangle_type == terrain_map::triangle_downhill;
					return { next_tile, bottom_left, next_tri };
				}
			case bottom_left:
				if (left_tri)
				{
					// go to next tile
					const auto next_tile = pos - tile_position{ 1, 0 };
					if (next_tile.x < 0)
						return bad_specific_vertex;

					if constexpr (BlockOnCliff)
					{
						const auto next_info = get_cliff_info(next_tile, m);
						if (next_info.right) // blocked by cliff
							return bad_specific_vertex;
					}

					return { next_tile, bottom_right, right_triangle };
				}
				else
				{
					// move to left tri
					assert(uphill); // there is no right_triangle for this corner in downhill
					if constexpr (BlockOnCliff)
					{
						if (cliff_info.diag) // blocked by cliff
							return bad_specific_vertex;
					}
					return { pos, corner, left_triangle };
				}
			case bottom_right:
				// TODO: this if statement looks a little wiggly
				//		I feel like their is a better way to write this
				if (left_tri || uphill)
				{
					if constexpr (BlockOnCliff)
					{
						if (cliff_info.bottom) // blocked by cliff
							return bad_specific_vertex;
					}

					// go to next tile
					const auto next_tile = pos + tile_position{ 0, 1 };
					if (next_tile.y >= world_size.y)
						return bad_specific_vertex;

					const auto next_info = get_cliff_info(next_tile, m);
					const auto next_tri = next_info.triangle_type == terrain_map::triangle_uphill;
					return { next_tile, top_right, next_tri };
				}
				else if (!left_tri)
				{
					assert(!uphill); // right_tri only exists for downhill on this corner
					if constexpr (BlockOnCliff)
					{
						if (cliff_info.diag) // blocked by cliff
							return bad_specific_vertex;
					}
					return { pos, corner, left_triangle };
				}
				else
				{
					//assert(false); // unreachable
					throw logic_error{ "unreachable" };
				}
			case top_right:
				if (left_tri)
				{
					assert(uphill); // downhill doesn't have a right_tri on this corner
					if constexpr (BlockOnCliff)
					{
						if (cliff_info.diag) // blocked by cliff
							return bad_specific_vertex;
					}
					return { pos, corner, right_triangle };
				}
				else
				{
					if constexpr (BlockOnCliff)
					{
						if (cliff_info.right) // blocked by cliff
							return bad_specific_vertex;
					}
					const auto next_tile = pos + tile_position{ 1, 0 };
					if (next_tile.x >= world_size.x)
						return bad_specific_vertex;
					return { next_tile, top_left, left_triangle };
				}
			default:
				throw logic_error{ "invalid corner value passed to next_specific_vertex_anti_clockwise" };
			}
		}

		// returns the height of a specific vertex
		inline std::uint8_t get_specific_height(const terrain_map& m, const specific_vertex& vertex)
		{
			const auto& [p, c, t] = vertex;
			const auto index = to_tile_index(p, m);
			const auto height = get_triangle_height(index, m);
			const auto& t_info = get_cliff_info(p, m);
			const auto [first, second] = quad_index_to_triangle_index(c, t_info.triangle_type);
			const auto vertex_index = t ? first : second;
			return height[vertex_index];
		}

		inline void set_specific_height(terrain_map& m, const specific_vertex& vertex, const std::uint8_t new_h)
		{
			const auto& [p, c, t] = vertex;
			const auto index = to_tile_index(p, m);
			const auto height = get_triangle_height(index, m);
			const auto& t_info = get_cliff_info(p, m);
			const auto [first, second] = quad_index_to_triangle_index(c, t_info.triangle_type);
			const auto vertex_index = t ? first : second;
			height[vertex_index] = new_h;
			return;
		}

		std::uint8_t count_adjacent_cliffs(const terrain_map& m, const terrain_vertex_position p);

		template<bool BlockOnCliff = true, invocable_r<std::uint8_t, std::uint8_t> Func>
		void change_terrain_height_impl(const tile_position tile, const rect_corners corner,
			const bool left_triangle, terrain_map& m, const resources::terrain_settings& settings, Func&& f)
		{
			// check world limits
			const auto size = get_size(m);
			if (!within_world(tile, size))
				throw terrain_error{ "Invalid argument passed to change_terrain_height: 'tile' was outside the map" };

			const auto min_h = settings.height_min;
			const auto max_h = settings.height_max;
			const auto cliff_diff_min = settings.cliff_height_min;

			const auto specific_start = detail::specific_vertex{ tile, corner, left_triangle };

			// array of verticies to update
			auto verts = []() consteval {
				std::array<detail::specific_vertex, 8> out [[indeterminate]];
				out.fill(detail::bad_specific_vertex);
				return out;
				}();

			auto output_iter = std::begin(verts);
			*output_iter++ = specific_start;

			const auto start_h = detail::get_specific_height(m, specific_start);
			auto new_h = std::clamp(std::invoke(f, start_h), min_h, max_h);
			auto otherside_h = std::optional<std::uint8_t>{};
			auto specific_next = detail::next_specific_vertex_clockwise<BlockOnCliff>(specific_start, size, m);

			// rotate clockwise
			while (!detail::is_bad(specific_next) &&
				specific_next != specific_start)
			{
				*output_iter++ = specific_next;
				assert(detail::get_specific_height(m, specific_next) == start_h);
				specific_next = detail::next_specific_vertex_clockwise<BlockOnCliff>(specific_next, size, m);
			}

			// we bumped into a cliff or world edge (is_edge below will be true)
			if (detail::is_bad(specific_next))
			{
				const auto vert_pos = to_vertex_position(std::get<0>(specific_start), std::get<1>(specific_start));
				const auto adj_cliffs = count_adjacent_cliffs(m, vert_pos);
				const terrain_vertex_position map_size_vertex = get_vertex_size(m);
				const auto is_edge = edge_of_map(map_size_vertex, vert_pos);
				const auto has_cliffs = adj_cliffs == 2 || (adj_cliffs == 1 && is_edge);
				if constexpr (BlockOnCliff)
				{
					// check height on otherside of the cliff
					if (has_cliffs)
					{
						const auto otherside = detail::next_specific_vertex_clockwise<false>(*std::prev(output_iter), size, m);
						if (!detail::is_bad(otherside)) // wasn't actually a cliff, must be the worlds edge
							otherside_h = detail::get_specific_height(m, *std::prev(output_iter));
					}
				}

				// rotate anticlockwise
				specific_next = detail::next_specific_vertex_anti_clockwise<BlockOnCliff>(specific_start, size, m);
				// NOTE: We don't need to check against specific_start, we only
				//		go anti clockwise if the previous direction hit a cliff.
				//		We are guaranteed to hit a cliff at some point in this loop
				while (!detail::is_bad(specific_next))
				{
					*output_iter++ = specific_next;
					assert(detail::get_specific_height(m, specific_next) == start_h);
					specific_next = detail::next_specific_vertex_anti_clockwise(specific_next, size, m);
				}

				if constexpr (BlockOnCliff)
				{
					// check other cliff
					if (!otherside_h && has_cliffs)
					{
						const auto otherside = detail::next_specific_vertex_clockwise<false>(*std::prev(output_iter), size, m);
						if (!detail::is_bad(otherside)) // next may just be the worlds edge
							otherside_h = detail::get_specific_height(m, *std::prev(output_iter));
					}
				}
			}

			if constexpr (BlockOnCliff)
			{
				// check cliff height distance
				if (otherside_h)
				{
					// try to make sure difference between otherside_h and new_h is larger than 
					// cliff_min_distance
					const auto diff = new_h - *otherside_h;
					if (std::cmp_less(std::abs(diff), cliff_diff_min))
					{
						if (diff < 0)
							new_h = integer_clamp_cast<std::uint8_t>(*otherside_h - cliff_diff_min);
						else
							new_h = integer_clamp_cast<std::uint8_t>(*otherside_h + cliff_diff_min);
					}
				}
			}

			// set height on each specific_vert
			std::for_each(std::begin(verts), output_iter, [new_h, &m](const detail::specific_vertex& v) {
				detail::set_specific_height(m, v, new_h);
				return;
				});

			return;
		}
	}

	template<invocable_r<std::uint8_t, std::uint8_t> Func>
	void change_terrain_height(const tile_position tile, const rect_corners corner, const bool left_triangle, terrain_map& m, const resources::terrain_settings& settings, Func&& f)
	{
		return detail::change_terrain_height_impl<true>(tile, corner, left_triangle, m, settings, f);
	}

	template<invocable_r<std::uint8_t, std::uint8_t> Func>
	void change_terrain_height(const terrain_vertex_position vertex, terrain_map& m, const resources::terrain_settings& settings, Func&& f)
	{
		// adjust input and corner so we can address every vertex
		const auto world_size = get_size(m);
		auto position = tile_position{ vertex };
		auto corner = rect_corners::top_left;
		auto left_tri = true;

		if (position == world_size)
		{
			position -= tile_position{ 1, 1 };
			corner = rect_corners::bottom_right;
			left_tri = false;
		}
		else if (position.x == world_size.x)
		{
			--position.x;
			corner = rect_corners::top_right;
			left_tri = false;
		}
		else if (position.y == world_size.y)
		{
			--position.y;
			corner = rect_corners::bottom_left;
		}

		return detail::change_terrain_height_impl<false>(position, corner, left_tri, m, settings, f);
	}
}
