#include "hades/tiles.hpp"

namespace hades
{
	namespace detail
	{
		template<bool PassInvalid = true, typename Func>
		std::enable_if_t<std::is_invocable_v<Func, tile_index_t>> for_each_index_rect_impl(const tile_index_t pos,
			tile_position size, const tile_index_t map_width, const tile_index_t max_index, Func&& f)
			noexcept(std::is_nothrow_invocable_v<Func, tile_index_t>)
		{
			assert(size.x >= 0);
			assert(size.y >= 0);
			const auto pos_y = pos / map_width;
			const auto pos_x = pos - pos_y * map_width;
			for (auto y = tile_index_t{}; y < size.y; ++y)
			{
				for (auto x = tile_index_t{}; x < size.x; ++x)
				{
					if ((pos_y + y) * map_width >= max_index ||
						pos_x + x >= map_width)
					{
						if constexpr(PassInvalid)
							std::invoke(f, bad_tile_index);
					}
					else
						std::invoke(f, pos + x + y * map_width);
				}
			}
			return;
		}
	}

	template<typename Func>
	std::enable_if_t<std::is_invocable_v<Func, tile_position>> for_each_position_rect(const tile_position position,
		const tile_position size, const tile_position world_size, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_position>)
	{
		const auto func = [&f, width = world_size.x](tile_index_t i) {
			const auto tile = i == bad_tile_index ? bad_tile_position : hades::from_tile_index(i, width);
			std::invoke(f, tile);
			return;
		};

		return detail::for_each_index_rect_impl(hades::to_tile_index(position, world_size.x),
			size, world_size.x, world_size.x * world_size.y, func);
	}

	template<typename Func>
	std::enable_if_t<std::is_invocable_v<Func, tile_position>> for_each_safe_position_rect(const tile_position position,
		const tile_position size, const tile_position world_size, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_position>)
	{
		const auto func = [&f, width = world_size.x](tile_index_t i) {
			const auto tile = from_tile_index(i, width);
			std::invoke(f, tile);
			return;
		};

		return detail::for_each_index_rect_impl<false>(hades::to_tile_index(position, world_size.x),
			size, world_size.x, world_size.x * world_size.y, func);
	}

	template<typename Func>
	std::enable_if_t<std::is_invocable_v<Func, tile_index_t>> for_each_safe_index_rect(const tile_index_t pos,
		tile_position size, const tile_index_t map_width, const tile_index_t max_index, Func&& f)
		noexcept(std::is_nothrow_invocable_v<Func, tile_index_t>)
	{
		return detail::for_each_index_rect_impl(pos, size, map_width, max_index, std::forward<Func>(f));
	}


	template<typename Func>
	std::enable_if_t<std::is_invocable_v<Func, tile_index_t>> for_each_index_rect(const tile_index_t pos,
		tile_position size, const tile_index_t map_width, const tile_index_t max_index, Func&& f)
		noexcept(std::is_nothrow_invocable_v<Func, tile_index_t>)
	{
		return detail::for_each_index_rect_impl<false>(pos, size, map_width, max_index, std::forward<Func>(f));
	}
}
