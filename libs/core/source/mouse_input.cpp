#include "hades/mouse_input.hpp"

#include <string_view>

#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Graphics/View.hpp"

#include "hades/data.hpp"
#include "hades/math.hpp"
#include "hades/timers.hpp"

namespace hades
{
	namespace input
	{
		input_id mouse_position = input_id::zero;
		input_id mouse_left = input_id::zero;
		input_id mouse_wheel = input_id::zero;
	}

	namespace mouse
	{
		bool inside_target(const sf::RenderTarget &t, vector_int pos)
		{
			const auto size = static_cast<sf::Vector2i>(t.getSize());
			const rect_t<vector_int::value_type> window{ 0, 0, size.x, size.y };
			return is_within(pos, window);
		}
		vector_float to_world_coords(const sf::RenderTarget &t, vector_int pos, const sf::View &v)
		{
			const auto r = t.mapPixelToCoords({ pos.x, pos.y }, v);
			return { r.x, r.y };
		}

		vector_int to_window_coords(const sf::RenderTarget &t, vector_float pos, const sf::View &v)
		{
			const auto r = t.mapCoordsToPixel({ pos.x, pos.y }, v);
			return { r.x, r.y };
		}

		vector_int snap_to_grid(vector_int coord, types::int32 grid_size)
		{
			const auto coordf = vector_float{ static_cast<vector_float::value_type>(coord.x),
										static_cast<vector_float::value_type>(coord.y) };

			const auto snap_pos = coordf -
				vector_float{ static_cast<vector_float::value_type>(std::abs(std::fmod(coordf.x, grid_size))),
				static_cast<vector_float::value_type>(std::abs((std::fmod(coordf.y, grid_size)))) };

			return { static_cast<vector_int::value_type>(snap_pos.x),
				static_cast<vector_int::value_type>(snap_pos.y) };
		}
	}

	void register_mouse_input(input_system &bind)
	{
		using namespace std::string_view_literals;
		constexpr auto m = "mouse"sv;
		constexpr auto m1 = "mouseleft"sv;
		constexpr auto mwheel = "mousewheel"sv;

		//collect unique names for actions
		input::mouse_position = data::make_uid(m);
		input::mouse_left = data::make_uid(m1);
		input::mouse_wheel = data::make_uid(mwheel);

		//bind them
		bind.create(input::mouse_position, false, m);
		bind.create(input::mouse_left, false, m1);
		bind.create(input::mouse_wheel, false, mwheel);
	}
}