#include "hades/mouse_input.hpp"

#include <string_view>

#include "SFML/Graphics/RenderTarget.hpp"
#include "SFML/Graphics/View.hpp"

#include "hades/data.hpp"
#include "hades/rectangle_math.hpp"
#include "hades/timers.hpp"

namespace hades
{
	namespace input
	{
		input_id mouse_position = input_id::zero;
		input_id mouse_left = input_id::zero;
		input_id mouse_right = input_id::zero;
		input_id mouse_wheel = input_id::zero;
	}

	namespace mouse
	{
		bool inside_target(const sf::RenderTarget &t, vector2_int pos)
		{
			const auto size = static_cast<sf::Vector2i>(t.getSize());
			const rect_t<vector2_int::value_type> window{ 0, 0, size.x, size.y };
			return is_within(pos, window);
		}

		vector2_float to_world_coords(const sf::RenderTarget &t, vector2_int pos, const sf::View &v)
		{
			const auto r = t.mapPixelToCoords({ pos.x, pos.y }, v);
			return { r.x, r.y };
		}

		vector2_int to_window_coords(const sf::RenderTarget &t, vector2_float pos, const sf::View &v)
		{
			const auto r = t.mapCoordsToPixel({ pos.x, pos.y }, v);
			return { r.x, r.y };
		}

		vector2_float snap_to_grid(vector2_float coord, float cell_size)
		{
			assert(cell_size != 0.f);
			const auto offset = vector2_float{
				std::abs(std::fmod(coord.x, cell_size)),
				std::abs(std::fmod(coord.y, cell_size))
			};
			
			return coord - offset;				
		}
	}

	void register_mouse_input(input_system &bind)
	{
		using namespace std::string_view_literals;
		constexpr auto m = "mouse"sv;
		constexpr auto m1 = "mouseleft"sv;
		constexpr auto m2 = "mouseright"sv;
		constexpr auto mwheel = "mousewheel"sv;

		//collect unique names for actions
		input::mouse_position = data::make_uid(m);
		input::mouse_left = data::make_uid(m1);
		input::mouse_right = data::make_uid(m2);
		input::mouse_wheel = data::make_uid(mwheel);

		//bind them
		bind.create(input::mouse_position, false, m);
		bind.create(input::mouse_left, false, m1);
		bind.create(input::mouse_right, false, m2);
		bind.create(input::mouse_wheel, false, mwheel);
	}
}