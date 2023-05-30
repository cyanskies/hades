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
			static_assert(std::is_invocable_v<Func, tile_index_t>);
			assert(pos >= 0);
			assert(size.x >= 0);
			assert(size.y >= 0);
			const auto pos_y = pos / map_width;
			const auto pos_x = pos - pos_y * map_width;
			for (auto y = tile_index_t{}; y < size.y; ++y)
			{
				for (auto x = tile_index_t{}; x < size.x; ++x)
				{
					if (const auto x_val = pos_x + x, y_val = pos_y + y;
						y_val * map_width >= max_index || // off the bottom of the map
						x_val >= map_width) // off the right edge
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

		template<bool PassInvalid = true, typename Func>
			requires std::is_invocable_v<Func, tile_index_t>
		void for_each_index_9_patch_impl(const tile_index_t pos,
			const tile_index_t map_width, const tile_index_t max_index, Func&& f)
			noexcept(std::is_nothrow_invocable_v<Func, tile_index_t>)
		{
			static_assert(std::is_invocable_v<Func, tile_index_t>);
			assert(pos >= 0);
			const auto pos_y = pos / map_width;
			const auto pos_x = pos - pos_y * map_width;

			for (auto y = -1; y < 2; ++y)
			{
				for (auto x = -1; x < 2; ++x)
				{
					if (x == 0 && y == 0)
						continue; // don't call on the middle

					if (const auto x_val = pos_x + x, y_val = pos_y + y;
						y_val * map_width >= max_index || // off the bottom of the map
						x_val >= map_width || // off the right edge
						x_val < 0 || y_val < 0) // off the top and left
					{
						if constexpr (PassInvalid)
							std::invoke(f, bad_tile_index);
					}
					else
						std::invoke(f, pos + x + y * map_width);
				}
			}
			return;
		}

		template<bool PassInvalid = true, typename Func>
			requires std::is_invocable_r_v<for_each_expanding_return, Func, tile_index_t>
		void for_each_expanding_index_impl(const tile_index_t pos,
			const tile_index_t map_width, const tile_index_t max_index, Func&& f)
			noexcept(std::is_nothrow_invocable_v<Func, tile_index_t>)
		{
			auto pos_y = (pos / map_width);
			auto pos_x = pos - pos_y * map_width;
			++pos_y; // start below the actual start pos
			auto edge_length = 1;
			auto edge_count = 0;
	
			enum class dir {
				north, east, south, west
			};

			auto d = dir::north;

			const auto outside_world =  [map_width, max_index] (tile_index_t pos_x, tile_index_t pos_y) noexcept {
				return pos_x < 0 || pos_y < 0 ||
					pos_x > map_width ||
					pos_x * pos_y > max_index;
			};

			const auto test_edge = [&] () noexcept(std::is_nothrow_invocable_v<Func, tile_index_t>) {
				while (edge_count++ != edge_length)
				{
					switch (d)
					{
					case dir::north:
						--pos_y;
						break;
					case dir::east:
						++pos_x;
						break;
					case dir::south:
						++pos_y;
						break;
					case dir::west:
						--pos_x;
					}

					if (std::invoke(outside_world, pos_x, pos_y))
					{
						if constexpr (PassInvalid)
						{
							if (std::invoke(f, bad_tile_index) == for_each_expanding_return::stop)
								return true;
						}
					}
					else
					{
						if (std::invoke(f, hades::to_1d_index({ pos_x, pos_y }, map_width)) == 
							for_each_expanding_return::stop)
							return true;
					}
				}

				edge_count = 0;
				return false;
			};


			while (true)
			{
				switch (d)
				{
				case dir::north:
				{
					if (std::invoke(test_edge))
						return;

					d = dir::east;
					break;
				}
				case dir::east:
				{
					--pos_x;
					--pos_y;
					++edge_length;

					if (std::invoke(test_edge))
						return;

					d = dir::south;
					break;
				}
				case dir::south:
				{
					if (std::invoke(test_edge))
						return;

					d = dir::west;
					break;
				}
				case dir::west:
				{
					if (std::invoke(test_edge))
						return;

					d = dir::north;
					break;
				}
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
			static_assert(std::is_invocable_v<std::decay_t<decltype(f)>, tile_position>);
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
			static_assert(std::is_invocable_v<std::decay_t<decltype(f)>, tile_position>);
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
		return detail::for_each_index_rect_impl<false>(pos, size, map_width, max_index, std::forward<Func>(f));
	}


	template<typename Func>
	std::enable_if_t<std::is_invocable_v<Func, tile_index_t>> for_each_index_rect(const tile_index_t pos,
		tile_position size, const tile_index_t map_width, const tile_index_t max_index, Func&& f)
		noexcept(std::is_nothrow_invocable_v<Func, tile_index_t>)
	{
		return detail::for_each_index_rect_impl(pos, size, map_width, max_index, std::forward<Func>(f));
	}

	template<typename Func>
		requires std::is_invocable_v<Func, tile_index_t>
	void for_each_safe_index_9_patch(const tile_index_t pos,
		const tile_index_t map_width, const tile_index_t max_index, Func&& f)
		noexcept(std::is_nothrow_invocable_v<Func, tile_index_t>)
	{
		return detail::for_each_index_9_patch_impl<false>(pos, map_width, max_index, std::forward<Func>(f));
	}

	template<typename Func>
		requires std::is_invocable_v<Func, tile_index_t>
	void for_each_index_9_patch(const tile_index_t pos,
		const tile_index_t map_width, const tile_index_t max_index, Func&& f)
		noexcept(std::is_nothrow_invocable_v<Func, tile_index_t>)
	{
		return detail::for_each_index_9_patch_impl(pos, map_width, max_index, std::forward<Func>(f));
	}

	template<typename Func>
		requires std::is_invocable_r_v<for_each_expanding_return, Func, tile_position>
	void for_each_safe_expanding_position(const tile_position position,
		const tile_position size, const tile_position world_size, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_position>)
	{
		const auto func = [&f, width = world_size.x](tile_index_t i) {
			static_assert(std::is_invocable_v<std::decay_t<decltype(f)>, tile_position>);
			const auto tile = from_tile_index(i, width);
			std::invoke(f, tile);
			return;
		};

		return detail::for_each_expanding_index_impl<false>(hades::to_tile_index(position, world_size.x),
			size, world_size.x, world_size.x * world_size.y, func);
	}
	
	template<typename Func>
		requires std::is_invocable_r_v<for_each_expanding_return, Func, tile_position>
	void for_each_expanding_position(const tile_position position,
		const tile_position size, const tile_position world_size, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_position>)
	{
		const auto func = [&f, width = world_size.x](tile_index_t i) {
			static_assert(std::is_invocable_v<std::decay_t<decltype(f)>, tile_position>);
			const auto tile = from_tile_index(i, width);
			std::invoke(f, tile);
			return;
		};

		return detail::for_each_expanding_index_impl(hades::to_tile_index(position, world_size.x),
			size, world_size.x, world_size.x * world_size.y, func);
	}

	template<typename Func>
		requires std::is_invocable_r_v<for_each_expanding_return, Func, tile_index_t>
	void for_each_safe_expanding_index(const tile_index_t pos, const tile_index_t map_width,
		const tile_index_t max_index, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_index_t>)
	{
		return detail::for_each_expanding_index_impl<false>(pos, map_width, max_index, std::forward<Func>(f));
	}

	template<typename Func>
		requires std::is_invocable_r_v<for_each_expanding_return, Func, tile_index_t>
	void for_each_expanding_index(const tile_index_t pos, const tile_index_t map_width,
		const tile_index_t max_index, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_index_t>)
	{
		return detail::for_each_expanding_index_impl(pos, map_width, max_index, std::forward<Func>(f));
	}
}
