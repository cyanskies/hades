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

		constexpr time_duration double_click_time = time_cast<time_duration>(seconds{ 0.30f });
		constexpr auto double_click_distance = 6;
		constexpr auto drag_distance = 6;

		void update_button_state(const action &m, const time_point &t, mouse_button_state &s)
		{
			const time_duration click_duration = s.is_down ? t - s.click_time : time_duration{};
			const auto position = vector_int{ m.x_axis, m.y_axis };
			const auto distance = vector::distance(position, s.click_pos);

			//mouse button is pressed
			if (m.active && !s.is_down)
			{
				//mouse has been clicked previously
				if (click_duration < double_click_time &&
					distance < double_click_distance &&
					s.enable_double_click)
				{
					s.double_clicked = true;
					s.clicked = false;
				}
				else
				{
					s.click_time = t;
					s.click_pos = position;
					s.clicked = true;
				}
			}
			else if(m.active && s.is_down)
			{
				//mouse is still down
				s.clicked = false;
				if (s.enable_drag)
				{
					if (distance > drag_distance &&
						!s.dragging)
					{
						s.drag_start = true;
					}
					else if (distance > drag_distance)
					{
						s.drag_start = false;
						s.dragging = true;
					}
				}
			}
			else if(!m.active && s.is_down)
			{
				//mouse has been released
				s.clicked = s.double_clicked = false;

				if (s.enable_drag && s.dragging || s.drag_start)
				{
					s.drag_start = s.dragging = false;
					s.drag_end = true;
				}
			}
			else
			{
				s.clicked = false;
				s.double_clicked = false;
				s.drag_start = false;
				s.dragging = false;
				s.drag_end = false;
			}

			s.is_down = m.active;
		}

		bool is_click(const mouse_button_state &s)
		{
			return s.clicked;
		}

		bool is_double_click(const mouse_button_state &s)
		{
			return s.double_clicked;
		}

		bool is_drag_start(const mouse_button_state &s)
		{
			return s.drag_start;
		}

		bool is_dragging(const mouse_button_state &s)
		{
			return s.dragging;
		}

		bool is_drag_end(const mouse_button_state &s)
		{
			return s.drag_end;
		}
	}

	void register_mouse_input(input_system &bind)
	{
		using namespace std::string_view_literals;
		constexpr auto m = "mouse"sv;
		constexpr auto m1 = "mouseleft"sv;

		//collect unique names for actions
		input::mouse_position = data::make_uid(m);
		input::mouse_left = data::make_uid(m1);

		//bind them
		bind.create(input::mouse_position, false, m);
		bind.create(input::mouse_left, false, m1);
	}
}