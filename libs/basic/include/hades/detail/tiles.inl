#include "hades/tiles.hpp"

namespace hades
{
	namespace detail
	{
		template<bool PassInvalid = true, std::invocable<tile_index_t> Func>
		void for_each_index_rect_impl(const tile_index_t pos, tile_position size,
			const tile_index_t map_width, const tile_index_t max_index, Func&& f)
			noexcept(std::is_nothrow_invocable_v<Func, tile_index_t>)
		{
			static_assert(std::is_invocable_v<Func, tile_index_t>);
			assert(pos >= 0);
			assert(size.x >= 0);
			assert(size.y >= 0);
			const auto pos_y = pos / map_width;
			const auto pos_x = pos - pos_y * map_width;
			const auto world_size = tile_position{ map_width, max_index / map_width };
			for (auto y = tile_index_t{}; y < size.y; ++y)
			{
				for (auto x = tile_index_t{}; x < size.x; ++x)
				{
					if (within_world({ pos_x + x, pos_y + y }, world_size))
						std::invoke(f, pos + x + y * map_width);
					else
					{
						if constexpr(PassInvalid)
							std::invoke(f, bad_tile_index);
					}	
				}
			}
			return;
		}

		template<bool PassInvalid = true, std::invocable<tile_position> Func>
		void for_each_pos_rect_impl(const tile_position pos, const tile_position size, 
			const tile_position world_size, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_position>)
		{
			const auto& [pos_x, pos_y] = pos;
			for (auto y = tile_index_t{}; y < size.y; ++y)
			{
				for (auto x = tile_index_t{}; x < size.x; ++x)
				{
					if (within_world({ pos_x + x, pos_y + y }, world_size))
						std::invoke(f, tile_position{ pos_x + x, pos_y + y });
					else
					{
						if constexpr (PassInvalid)
							std::invoke(f, bad_tile_position);
					}
				}
			}
			return;
		}

		template<bool PassInvalid = true, std::invocable<tile_position> Func>
		void for_each_position_circle_impl(const tile_position p, const tile_index_t radius,
			const tile_position world_size, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_position>)
		{
			const auto rad = integer_cast<tile_position::value_type>(radius);

			const auto top = tile_position{ 0, -rad };
			const auto bottom = tile_position{ 0, rad };

			const auto call_on_position = [&f, world_size](tile_position pos) 
				noexcept(std::is_nothrow_invocable_v<Func, tile_position>) {
				if (within_world(pos, world_size))
					std::invoke(f, pos);
				else
				{
					if constexpr (PassInvalid)
						std::invoke(f, bad_tile_position);
				}
			};

			std::invoke(call_on_position, top + p);

			const auto r2 = rad * rad;
			for (auto y = top.y + 1; y < bottom.y; ++y)
			{
				//find x for every y between the top and bottom of the circle

				//x2 + y2 = r2
				//x2 = r2 - y2
				//x = sqrt(r2 - y2)
				const auto y2 = y * y;
				const auto a = r2 - y2;
				assert(a >= 0);
				const auto x_root = std::sqrt(static_cast<float>(a));
				const auto x_float = std::trunc(x_root);

				const auto x_int = std::abs(static_cast<tile_position::value_type>(x_float));
				const auto bounds = std::array{ -x_int, x_int };

				//push the entire line of the circle into out
				for (auto x = bounds[0]; x <= bounds[1]; x++)
					std::invoke(call_on_position, tile_position{ x + p.x, y + p.y });
			}

			std::invoke(call_on_position, bottom + p);
			return;
		}

		template<bool PassInvalid = true, std::invocable<tile_index_t> Func>
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

					if (within_world({ pos_x + x, pos_y + y }, {map_width, max_index/map_width}))
						std::invoke(f, pos + x + y * map_width);
					else
					{
						if constexpr (PassInvalid)
							std::invoke(f, bad_tile_index);
					}	
				}
			}
			return;
		}

		template<bool PassInvalid = true, invocable_r<for_each_expanding_return, tile_index_t> Func>
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
				return !within_world({ pos_x, pos_y }, { map_width, max_index / map_width });
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

	constexpr bool within_world(const tile_position p, const tile_position w) noexcept
	{
		return p.y >= 0 && // off the top of the map
			p.y * w.x < (w.x * w.y) && // off the bottom of the map
			p.x >= 0 && /// off the left of the map
			p.x < w.x; // off the right of the map
	}

	template<std::invocable<tile_position> Func>
	void for_each_position_rect(const tile_position position, const tile_position size,
		const tile_position world_size, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_position>)
	{
		return detail::for_each_pos_rect_impl(position, size, world_size, std::forward<Func>(f));
	}

	template<std::invocable<tile_position> Func>
	void for_each_safe_position_rect(const tile_position position, const tile_position size,
		const tile_position world_size, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_position>)
	{
		return detail::for_each_pos_rect_impl<false>(position, size, world_size, std::forward<Func>(f));
	}

	template<std::invocable<tile_position> Func>
	void for_each_position_circle(tile_position middle, tile_index_t radius, tile_position world_size, Func&& f)
		noexcept(std::is_nothrow_invocable_v<Func, tile_position>)
	{
		return detail::for_each_position_circle_impl(middle, radius, world_size, std::forward<Func>(f));
	}

	template<std::invocable<tile_position> Func>
	void for_each_safe_position_circle(tile_position middle, tile_index_t radius, tile_position world_size, Func&& f) 
		noexcept(std::is_nothrow_invocable_v<Func, tile_position>)
	{
		return detail::for_each_position_circle_impl<false>(middle, radius, world_size, std::forward<Func>(f));
	}

	namespace detail
	{
		template<bool PassInvalid = true, std::invocable<tile_position> Func>
		void for_each_position_line_impl(tile_position start, const tile_position end, const tile_position world_size, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_position>)
		{
			// axis aligned lines
			assert(start.x == end.x ||
				start.y == end.y);

			auto diff = tile_position{};
			if (start.x == end.x)
			{
				assert(start.y != end.y);
				diff.y = start.y < end.y ? 1 : -1;
			}
			else
			{
				assert(start.x != end.x);
				diff.y = start.x < end.x ? 1 : -1;
			}

			while (start != end)
			{
				if (within_world(start, world_size))
					std::invoke(f, start);
				else if constexpr (PassInvalid)
					std::invoke(f, bad_tile_position);

				start += diff;
			}
			return;
		}

		template<bool PassInvalid = true, std::invocable<tile_position> Func>
		void for_each_position_diagonal_impl(tile_position start, const tile_position end, const tile_position world_size, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_position>)
		{
			const auto x = start.x - end.x;
			const auto y = start.y - end.y;
			assert(abs(x) == abs(y));

			const auto diff = tile_position{
				x / abs(x),
				y / abs(y)
			};

			while (start != end)
			{
				if (within_world(start, world_size))
					std::invoke(f, start);
				else if constexpr (PassInvalid)
					std::invoke(f, bad_tile_position);

				start += diff;
			}
			return;
		}
	}

	template<std::invocable<tile_position> Func>
	void for_each_position_line(const tile_position start, const tile_position end, const tile_position world_size, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_position>)
	{
		return detail::for_each_position_line_impl(start, end, world_size, std::forward<Func>(f));
	}

	template<std::invocable<tile_position> Func>
	void for_each_safe_position_line(const tile_position start, const tile_position end, const tile_position world_size, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_position>)
	{
		return detail::for_each_position_line_impl<false>(start, end, world_size, std::forward<Func>(f));
	}

	template<std::invocable<tile_position> Func>
	void for_each_position_diagonal(tile_position const start, tile_position const end, tile_position const world_size, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_position>)
	{
		return detail::for_each_position_diagonal_impl(start, end, world_size, std::forward<Func>(f));
	}

	template<std::invocable<tile_position> Func>
	void for_each_safe_position_diagonal(const tile_position start, const tile_position end, const tile_position world_size, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_position>)
	{
		return detail::for_each_position_diagonal_impl<false>(start, end, world_size, std::forward<Func>(f));
	}

	template<std::invocable<tile_index_t> Func>
	void for_each_safe_index_rect(const tile_index_t pos, tile_position size,
		const tile_index_t map_width, const tile_index_t max_index, Func&& f)
		noexcept(std::is_nothrow_invocable_v<Func, tile_index_t>)
	{
		return detail::for_each_index_rect_impl<false>(pos, size, map_width, max_index, std::forward<Func>(f));
	}

	template<std::invocable<tile_index_t> Func>
	void for_each_index_rect(const tile_index_t pos, tile_position size,
		const tile_index_t map_width, const tile_index_t max_index, Func&& f)
		noexcept(std::is_nothrow_invocable_v<Func, tile_index_t>)
	{
		return detail::for_each_index_rect_impl(pos, size, map_width, max_index, std::forward<Func>(f));
	}

	template<std::invocable<tile_index_t> Func>
	void for_each_safe_index_9_patch(const tile_index_t pos,
		const tile_index_t map_width, const tile_index_t max_index, Func&& f)
		noexcept(std::is_nothrow_invocable_v<Func, tile_index_t>)
	{
		return detail::for_each_index_9_patch_impl<false>(pos, map_width, max_index, std::forward<Func>(f));
	}

	template<std::invocable<tile_index_t> Func>
	void for_each_index_9_patch(const tile_index_t pos,
		const tile_index_t map_width, const tile_index_t max_index, Func&& f)
		noexcept(std::is_nothrow_invocable_v<Func, tile_index_t>)
	{
		return detail::for_each_index_9_patch_impl(pos, map_width, max_index, std::forward<Func>(f));
	}

	template<invocable_r<for_each_expanding_return, tile_position> Func>
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
			size, world_size.x, world_size.x * world_size.y, std::forward<Func>(f));
	}
	
	template<invocable_r<for_each_expanding_return, tile_position> Func>
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
			size, world_size.x, world_size.x * world_size.y, std::forward<Func>(f));
	}

	template<invocable_r<for_each_expanding_return, tile_index_t> Func>
	void for_each_safe_expanding_index(const tile_index_t pos, const tile_index_t map_width,
		const tile_index_t max_index, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_index_t>)
	{
		return detail::for_each_expanding_index_impl<false>(pos, map_width, max_index, std::forward<Func>(f));
	}

	template<invocable_r<for_each_expanding_return, tile_index_t> Func>
	void for_each_expanding_index(const tile_index_t pos, const tile_index_t map_width,
		const tile_index_t max_index, Func&& f) noexcept(std::is_nothrow_invocable_v<Func, tile_index_t>)
	{
		return detail::for_each_expanding_index_impl(pos, map_width, max_index, std::forward<Func>(f));
	}
}
