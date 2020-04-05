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
		extern input_id mouse_right;
		extern input_id mouse_wheel;
	}

	namespace mouse
	{
		namespace detail
		{
			// Any way to avoid this
			struct empty {};
			struct empty2 {};
			struct empty3 {};

			struct click_pos
			{
				vector_int click_pos;
			};

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
		}

		template<bool EnableDrag = true, bool EnableDoubleClick = true>
		struct mouse_button_state :
			public std::conditional_t<EnableDrag || EnableDoubleClick, detail::click_pos, detail::empty>,
			public std::conditional_t<EnableDoubleClick, detail::double_click_data, detail::empty2>,
			public std::conditional_t<EnableDrag, detail::drag_data, detail::empty3>
		{
			bool is_down = false;
			bool clicked = false;
		};

		bool inside_target(const sf::RenderTarget&, vector_int window_position);
		vector_float to_world_coords(const sf::RenderTarget&, vector_int window_position, const sf::View&);
		vector_int to_window_coords(const sf::RenderTarget&, vector_float world_position, const sf::View&);
		vector_float snap_to_grid(vector_float coord, float cell_size);

		//call update_button_state each update frame to keep a mouse_button_state current for each button you care about
		//use the is_* functions to check whether to trigger a mouse event
		template<bool Drag, bool DoubleClick>
		constexpr void update_button_state(const action &mouse_button, const action &mouse_position, const time_point &time, mouse_button_state<Drag, DoubleClick> &mouse_state) noexcept;
		
		//if true, then the position of the click is the same as the mouse position given to update_button_state
		template<bool Drag, bool DoubleClick>
		constexpr bool is_click(const mouse_button_state<Drag, DoubleClick> &mouse_state) noexcept;
		//if true, the location of the click is stored into mouse_state.click_pos
		template<bool Drag, bool DoubleClick>
		constexpr bool is_double_click(const mouse_button_state<Drag, DoubleClick> &mouse_state) noexcept;
		template<bool Drag, bool DoubleClick>
		constexpr vector_int double_click_pos(const mouse_button_state<Drag, DoubleClick>& mouse_state) noexcept;
		template<bool Drag, bool DoubleClick>
		constexpr bool is_drag_start(const mouse_button_state<Drag, DoubleClick> &mouse_state) noexcept;
		template<bool Drag, bool DoubleClick>
		constexpr bool is_dragging(const mouse_button_state<Drag, DoubleClick> &mouse_state) noexcept;
		template<bool Drag, bool DoubleClick>
		constexpr bool is_drag_end(const mouse_button_state<Drag, DoubleClick> &mouse_state) noexcept;
		template<bool Drag, bool DoubleClick>
		constexpr vector_int drag_start_pos(const mouse_button_state<Drag, DoubleClick>& mouse_state) noexcept;
	}

	//optional function for registering common input methods
	//such as mouse position and buttons:
	// mouse - mouse position
	// mouseleft - mouse button 1
	// mouseright - mouse button 2
	// mousewheel - mouse wheel axis
	void register_mouse_input(input_system &bind);
}

#include "detail/mouse_input.inl"

#endif //!HADES_MOUSE_INPUT_HPP
