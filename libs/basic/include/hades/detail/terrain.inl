#include "hades/terrain.hpp"

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
		struct add_height_functor
		{
			const std::uint8_t amount;

			constexpr std::uint8_t operator()(const std::uint8_t h) const noexcept
			{
				return integer_clamp_cast<std::uint8_t>(h + amount);
			}
		};
		
		struct sub_height_functor
		{
			const std::uint8_t amount;

			constexpr std::uint8_t operator()(const std::uint8_t h) const noexcept
			{
				return integer_clamp_cast<std::uint8_t>(h - amount);
			}
		};

		struct set_height_functor
		{
			const std::uint8_t height;

			constexpr std::uint8_t operator()(const std::uint8_t) const noexcept
			{
				return height;
			}
		};
	}

	// Converts base 6 triangle index's into quad corner indexes
	// Triangle indicies are defined in struct terrain_map (terrain.hpp)
	// Quad corners are defined in enum rect_corners (rectangle_math.hpp)
	constexpr rect_corners triangle_index_to_quad_index(const std::uint8_t i, const terrain_map::triangle_type tri_type) noexcept
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

		if (tri_type == terrain_map::triangle_type::triangle_uphill)
			return rect_corners{ uphill[i] };
		else
			return rect_corners{ downhill[i] };
	}

	constexpr std::pair<std::uint8_t, std::uint8_t> quad_index_to_triangle_index(const rect_corners c, const terrain_map::triangle_type tri_type) noexcept
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

		assert(c < rect_corners::end);

		if (tri_type == terrain_map::triangle_type::triangle_uphill)
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

	[[nodiscard]]
	constexpr std::array<terrain_vertex_position, 4> to_vertex_positions(const tile_position p) noexcept
	{
		return {
			to_vertex_position(p, rect_corners::top_left),
			to_vertex_position(p, rect_corners::top_right),
			to_vertex_position(p, rect_corners::bottom_right),
			to_vertex_position(p, rect_corners::bottom_left),
		};
	}

	constexpr terrain_map::triangle_type pick_triangle_type(const tile_position p) noexcept
	{
		if (p.y % 2 == 0)
			return terrain_map::triangle_type{ p.x % 2 == 0 };
		else
			return terrain_map::triangle_type{ p.x % 2 != 0 };
	}

	constexpr std::array<std::uint8_t, 2> get_height_for_top_edge(const cell_height_data& c) noexcept
	{
		return {
			c[enum_type(rect_corners::top_left)],
			c[enum_type(rect_corners::top_right)]
		};
	}

	constexpr std::array<std::uint8_t, 2> get_height_for_left_edge(const cell_height_data& c) noexcept
	{
		return {
			c[enum_type(rect_corners::top_left)],
			c[enum_type(rect_corners::bottom_left)]
		};
	}

	constexpr std::array<std::uint8_t, 2> get_height_for_right_edge(const cell_height_data& c) noexcept
	{
		return {
			c[enum_type(rect_corners::top_right)],
			c[enum_type(rect_corners::bottom_right)]
		};
	}

	constexpr std::array<std::uint8_t, 2> get_height_for_bottom_edge(const cell_height_data& c) noexcept
	{
		return {
			c[enum_type(rect_corners::bottom_left)],
			c[enum_type(rect_corners::bottom_right)]
		};
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

	template<invocable_r<std::uint8_t, std::uint8_t> Func>
	void change_terrain_height(const terrain_vertex_position vertex, terrain_map& m, const resources::terrain_settings& settings, Func&& f)
	{
		const auto index = to_vertex_index(vertex, m);
		assert(index < size(m.heightmap));
		m.heightmap[index] = std::clamp(std::invoke(f, m.heightmap[index]), settings.height_min, settings.height_max);
		return;
	}

	namespace detail
	{
		inline void regenerate_cliff_tiles(tile_position pos, terrain_map& m,
			const resources::terrain* face_terrain, const resources::terrain* surface_terrain,
			const resources::terrain_settings& settings)
		{
			assert(face_terrain);
			assert(surface_terrain);

			const auto& empty = resources::get_empty_tile(settings);
			const auto empty_terrain = resources::get_empty_terrain(settings);
			const auto empty_id = make_tile_id(m.cliff_tiles, empty, empty_terrain);

			using cliff_layout = terrain_map::cliff_layer_layout;
			const auto index = integer_cast<std::size_t>(to_tile_index(pos, m)) * enum_type(cliff_layout::multiplier);

			const auto cliffs = get_adjacent_cliffs(pos, m);
			for (auto i = cliff_layout::top; i != cliff_layout::surface; i = next(i))
			{
				const auto face_index = index + enum_type(i);
				if (cliffs.test(enum_type(i)))
				{
					const auto tile = resources::get_random_tile(*face_terrain, resources::transition_tile_type::all, settings);
					const auto tile_id = make_tile_id(m.cliff_tiles, tile, face_terrain);
					m.cliff_tiles.tiles[face_index] = tile_id;
				}
				else
				{
					m.cliff_tiles.tiles[face_index] = empty_id;
				}
			}

			const auto surface_index = index + enum_type(cliff_layout::surface);
			const auto cliff_corners = get_cliff_corners(pos, m);
			const auto surface_tile = get_transition_type(cliff_corners);
			const auto tile = resources::get_random_tile(*surface_terrain, surface_tile, settings);
			// NOTE: if surface_tile == NONE then get_random_tile returns empty_tile
			//		we should technically be passing empty_terrain to make_tile_id
			//		in this case, however empty_terrain will always be loaded in the tilemap
			//		because we called make_tile_id with it above, so this call
			//		will always succeed
			const auto tile_id = make_tile_id(m.cliff_tiles, tile, surface_terrain);
			m.cliff_tiles.tiles[surface_index] = tile_id;
		}

		inline void regenerate_nearby_cliff_tiles(const tile_position pos, terrain_map& m, const resources::terrain_settings& settings)
		{
			constexpr auto adj_tiles = std::array{
				tile_position{ -1, -1 },	tile_position{ 0, -1},	tile_position{1, -1},
				tile_position{ -1, 0 },		tile_position{0, 0,},	tile_position{1, 0},
				tile_position{ -1, 1 },		tile_position{0, 1},	tile_position{1, 1}
			};

			const auto world_size = get_size(m);

			assert(m.terrainset->cliff_type);
			const auto cliff_terrain = m.terrainset->cliff_type.get();
			const auto face_terrain = cliff_terrain->cliff_face_terrain.get();
			const auto surface_terrain = cliff_terrain->cliff_surface_terrain.get();
			// regen adjacent tiles
			for (const auto& tile : adj_tiles)
			{
				const auto t = pos + tile;
				if (within_world(t, world_size))
					detail::regenerate_cliff_tiles(t, m, face_terrain, surface_terrain, settings);
			}
		}
	}

	template<invocable_r<std::uint8_t, std::uint8_t> Func>
	void change_terrain_cliff_layer(const tile_position pos, terrain_map& m, const resources::terrain_settings& settings, Func&& f)
	{
		const auto index = to_tile_index(pos, m);
		assert(index < size(m.cliff_layer));
		const auto starting_layer = m.cliff_layer[index];
		const auto our_layer = m.cliff_layer[index] = std::clamp(std::invoke(f, m.cliff_layer[index]), settings.cliff_min, settings.cliff_max);
		if (starting_layer != our_layer)
		{
			clear_ramp(pos, m, settings);
			detail::regenerate_nearby_cliff_tiles(pos, m, settings);
		}

		return;
	}
}
