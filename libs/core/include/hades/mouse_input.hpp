#ifndef HADES_MOUSE_INPUT_HPP
#define HADES_MOUSE_INPUT_HPP

#include "hades/input.hpp"
#include "hades/timers.hpp"
#include "hades/types.hpp"
#include "hades/uniqueid.hpp"
#include "hades/vector_math.hpp"

namespace sf
{
	class RenderTarget;
	class View;
}

namespace hades
{
	namespace input
	{
		using input_id = hades::unique_id;

		extern input_id mouse_position;
		extern input_id mouse_left;
	}

	namespace mouse
	{
		struct mouse_button_state
		{
			mouse_button_state() noexcept = default;
			mouse_button_state(bool double_click, bool drag) noexcept
				: enable_double_click{ double_click }, enable_drag{ drag }
			{}

			vector_int prev_down_location;
			time_point prev_down_time;
			bool prev_down = false;
			bool is_down = false;
			bool enable_double_click = true;
			bool enable_drag = true;
		};

		struct never_double_click_t {};
		constexpr auto never_double_click = never_double_click_t{};

		bool inside_target(const sf::RenderTarget&, vector_int window_position);
		vector_float to_world_coords(const sf::RenderTarget&, vector_int window_position, const sf::View&);
		vector_int to_window_coords(const sf::RenderTarget&, vector_float world_position, const sf::View&);
		vector_int snap_to_grid(vector_int coord, types::int32 grid_size);
		bool is_click(const action &mouse_button, mouse_button_state &mouse_state);
		bool is_double_click(const action &mouse_button, mouse_button_state &mouse_state);
		bool is_dragging(const action &mouse_button, mouse_button_state &mouse_state);
		bool is_drag_end(const action &mouse_button, mouse_button_state &mouse_state);
		//TODO: within window
	}

	//optional function for registering common input methods
	//such as mouse position and buttons:
	// mouse - mouse position
	// mouseleft - mouse button 1
	void register_mouse_input(input_system &bind);
}

#endif !//HADES_MOUSE_INPUT_HPP
