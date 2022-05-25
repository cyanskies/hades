#include "hades/level_editor_grid.hpp"

#include "hades/mouse_input.hpp"

namespace hades
{
	template<typename T>
	vector_t<T> snap_to_grid(vector_t<T> p, const grid_vars&g)
	{
		if (g.snap->load())
		{
			const auto cell_size = calculate_grid_size(*g.step);

			const auto snap_float_p = mouse::snap_to_grid(static_cast<vector_float>(p), cell_size);

			return static_cast<vector_t<T>>(snap_float_p);
		}

		return p;
	}
}
