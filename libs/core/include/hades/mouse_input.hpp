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
		template<bool EnableDrag = true, bool EnableDoubleClick = true>
		struct mouse_button_state
		{
			struct disabled
			{};

			struct double_click_data
			{
				time_point click_time;
				bool double_clicked = false;
			};

			struct drag_data
			{
				bool drag_start = false;
				bool dragging = false;
				bool drag_end = false;
			};

			bool is_down = false;
			bool clicked = false;

			std::conditional_t<EnableDrag || EnableDoubleClick, vector_int, disabled> click_pos;
			std::conditional_t<EnableDoubleClick, double_click_data, disabled> double_clicked;
			std::conditional_t<EnableDrag, drag_data, disabled> drag;
		};

		bool inside_target(const sf::RenderTarget&, vector_int window_position);
		vector_float to_world_coords(const sf::RenderTarget&, vector_int window_position, const sf::View&);
		vector_int to_window_coords(const sf::RenderTarget&, vector_float world_position, const sf::View&);
		vector_int snap_to_grid(vector_int coord, types::int32 grid_size);

		//call update_button_state each update frame to keep a mouse_button_state current for each button you care about
		//use the is_* functions to check whether to trigger a mouse event
		template<bool Drag, bool DoubleClick>
		void update_button_state(const action &mouse_button, const action &mouse_position, const time_point &time, mouse_button_state<Drag, DoubleClick> &mouse_state);
		
		template<bool Drag, bool DoubleClick>
		bool is_click(const mouse_button_state<Drag, DoubleClick> &mouse_state);
		template<bool Drag, bool DoubleClick>
		bool is_double_click(const mouse_button_state<Drag, DoubleClick> &mouse_state);
		template<bool Drag, bool DoubleClick>
		bool is_drag_start(const mouse_button_state<Drag, DoubleClick> &mouse_state);
		template<bool Drag, bool DoubleClick>
		bool is_dragging(const mouse_button_state<Drag, DoubleClick> &mouse_state);
		template<bool Drag, bool DoubleClick>
		bool is_drag_end(const mouse_button_state<Drag, DoubleClick> &mouse_state);
	}

	//optional function for registering common input methods
	//such as mouse position and buttons:
	// mouse - mouse position
	// mouseleft - mouse button 1
	void register_mouse_input(input_system &bind);
}

#include "detail/mouse_input.inl"

#endif //!HADES_MOUSE_INPUT_HPP
