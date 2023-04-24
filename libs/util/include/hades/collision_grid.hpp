#ifndef HADES_COLLISION_GRID_HPP
#define HADES_COLLISION_GRID_HPP

#include <vector>
#include <unordered_map>

namespace hades
{
	/// @brief Collision detection using a uniform grid accross the world
	template<typename Key, typename Rect>
	class uniform_collision_grid
	{
	public:
		using key_type = Key;
		using rect_type = Rect;

		/// @brief 
		///		Create a new uniform grid covering world_bounds will cell_size cells.
		/// @param world_bounds: 
		///		The world area to be covered by the grid, supports negative coords
		/// @param cell_size:
		///		The size of the cells will dictate how many cells are needed
		///		to cover the whole world. If the cell_size is not evenly divisible
		///		by the world size, then extra cells will be used to fill the gaps.
		uniform_collision_grid(rect_type world_bounds, typename rect_type::value_type cell_size);

		void insert(key_type, rect_type);
		void update(key_type, rect_type);
		void remove(key_type);
		
		[[nodiscard]] std::vector<key_type> find(rect_type) const;

	private:
		static constexpr std::size_t bad_node =
			std::numeric_limits<std::size_t>::max();

		struct node
		{
			key_type key;
			std::size_t next = bad_node;
		};

		void _append(std::size_t cell, key_type k);
		void _remove(std::size_t cell, key_type k);

		// calls UnaryFunc on each cell_index that intersects rect
		template<typename UnaryFunc>
		void _for_each_found_cell(rect_type rect, UnaryFunc&& f) const
			noexcept(std::is_nothrow_invocable_v<UnaryFunc, std::size_t>);

		// main grid data
		std::vector<std::size_t> _empty_index;
		std::vector<node> _nodes;

		// lookup by id info
		std::unordered_map<key_type, rect_type> _rects;

		// cell scaling info
		typename rect_type::value_type _world_x, _world_y;
		typename rect_type::value_type _cell_size;
		std::size_t _cells_per_row;
		std::size_t _cell_count;
	};
}

#include "hades/detail/collision_grid.inl"

#endif // !HADES_COLLISION_GRID_HPP
