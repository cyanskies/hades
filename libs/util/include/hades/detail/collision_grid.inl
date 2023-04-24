#include "hades/collision_grid.hpp"

#include "hades/rectangle_math.hpp"
#include "hades/utility.hpp"

namespace hades
{
	template<typename Key, typename Rect>
	uniform_collision_grid<Key, Rect>::uniform_collision_grid(const rect_type w_rect, const typename rect_type::value_type cell_size)
		: _cell_size{ cell_size }, _nodes{}, _empty_index{}
	{
		const auto world_rect = normalise(w_rect);
		_world_x = world_rect.x;
		_world_y = world_rect.y;

		// calculate the number of cells
		const auto cell_sizef = static_cast<float>(cell_size);
		const auto row_count = world_rect.width / cell_sizef;
		_cells_per_row = static_cast<std::size_t>(std::round(row_count));
		const auto height_count = world_rect.height / cell_sizef;
		_cell_count = static_cast<std::size_t>(std::round(height_count)) * _cells_per_row;

		_nodes.resize(_cell_count);
		return;
	}

	template<typename Key, typename Rect>
	void uniform_collision_grid<Key, Rect>::insert(const key_type k, const rect_type rect)
	{
		const auto rect2 = normalise(rect);
		_for_each_found_cell(rect2, [&](auto c) {
			_append(c, k);
			return;
			});

		_rects.emplace(k, rect2);
		return;
	}

	template<typename Key, typename Rect>
	void uniform_collision_grid<Key, Rect>::update(const key_type k, const rect_type rect)
	{
		remove(k);
		insert(k, rect);
		return;
	}

	template<typename Key, typename Rect>
	void uniform_collision_grid<Key, Rect>::remove(const key_type k)
	{
		const auto r = _rects.find(k);
		if (r == end(_rects))
			return;

		_for_each_found_cell(r->second, [&](auto c) {
			_remove(c, k);
			return;
			});

		_rects.erase(r);
		return;
	}

	template<typename Key, typename Rect>
	std::vector<typename uniform_collision_grid<Key, Rect>::key_type> uniform_collision_grid<Key, Rect>::find(rect_type r) const
	{
		auto out = std::vector<key_type>{};

		_for_each_found_cell(normalise(r), [&](auto c) {
			auto index = _nodes[c].next;
			while (index != bad_node)
			{
				out.emplace_back(_nodes[index].key);
				index = _nodes[index].next;
			}
			return;
			});

		remove_duplicates(out);
		return out;
	}

	template<typename Key, typename Rect>
	void uniform_collision_grid<Key, Rect>::_append(const std::size_t cell, const key_type k)
	{
		//create new node
		auto index = bad_node;
		if (empty(_empty_index))
		{
			_nodes.emplace_back(node{ k });
			index = size(_nodes) - 1;
		}
		else
		{
			index = _empty_index.back();
			_empty_index.pop_back();
			_nodes[index] = node{ k };
		}

		//insert at the start of the forward list
		_nodes[index].next = _nodes[cell].next;
		_nodes[cell].next = index;
		return;
	}

	template<typename Key, typename Rect>
	void uniform_collision_grid<Key, Rect>::_remove(const std::size_t cell, const key_type k)
	{
		auto prev = &_nodes[cell];
		assert(prev->next != bad_node); // we arent in this cell
		auto curr = &_nodes[prev->next];

		while (curr->key != k)
		{
			prev = curr;
			assert(curr->next != bad_node);
			curr = &_nodes[curr->next];
		}

		_empty_index.emplace_back(prev->next);
		prev->next = curr->next;

		return;
	}

	template<typename Key, typename Rect>
	template<typename UnaryFunc>
	void uniform_collision_grid<Key, Rect>::_for_each_found_cell(rect_type rect, UnaryFunc&& f) const
		noexcept(std::is_nothrow_invocable_v<UnaryFunc, std::size_t>)
	{
		// find the cells intersected by rect
		// move the world to (0,0)
		rect.x -= _world_x;
		rect.y -= _world_y;

		assert(rect.x >= 0);
		assert(rect.y >= 0);
		// shrink to cell size
		const auto& cs = hades::float_cast(_cell_size);
		
		// we reduce the values to a fraction of their origional values
		// we need to store these as floats or we lose a lot of the meaningful data
		const auto cell_rect = hades::rect_float{ 
			hades::float_cast(rect.x) / cs, hades::float_cast(rect.y) / cs,
			(hades::float_cast(rect.width) + cs) / cs, (hades::float_cast(rect.height) + cs) / cs 
		};

		const auto end_x = std::min(
			hades::integral_cast<std::size_t>(cell_rect.x + cell_rect.width, hades::round_down_tag),
			_cells_per_row);
		const auto columns = _cell_count / _cells_per_row;
		const auto end_y = std::min(
			hades::integral_cast<std::size_t>(cell_rect.y + cell_rect.height, hades::round_down_tag),
			columns);

		for (auto y = std::max(std::size_t{}, hades::integral_cast<std::size_t>(cell_rect.y, hades::round_down_tag)); y < end_y; ++y)
		{
			for (auto x = std::max(std::size_t{}, hades::integral_cast<std::size_t>(cell_rect.x, hades::round_down_tag)); x < end_x; ++x)
			{
				const auto cell_index = to_1d_index({ x, y }, _cells_per_row);
				assert(cell_index < _cell_count);
				std::invoke(f, cell_index);
			}
		}

		return;
	}
}
