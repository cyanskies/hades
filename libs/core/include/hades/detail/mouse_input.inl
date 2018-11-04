namespace hades::mouse
{
	constexpr time_duration double_click_time = time_cast<time_duration>(seconds{ 0.30f });
	constexpr auto double_click_distance = 6;
	constexpr auto drag_distance = 6;

	template<bool Drag, bool DoubleClick>
	inline void update_button_state(const action &m, const time_point &t, mouse_button_state<Drag, DoubleClick> &s)
	{
		//check each combination of inputs
		//mouse is down / not down
		//mouse was down / wans't down last frame

		//mouse button is pressed
		//NOTE: start a click, or issue a double click if we recently clicked
		//also record info need about the mouse down, for other state checks in later frames
		if (m.active && !s.is_down)
		{
			//TODO: FIXME: constexpr if doesn't work well in lambdas
			// this fails to compile if Drag and DoubleClick are false
			auto do_click_logic = [&s, &m] {
				if constexpr(Drag || DoubleClick)
					s.click_pos = vector_int{ m.x_axis, m.y_axis };

				s.clicked = true;
			};

			//NOTE: if doubleclick is disabled, then clicks can be generated quickly in sucession
			// otherwise each click must be at least double_click_time apart
			//TODO: investigate changing this, disabling DoubleClick shouldn't
			// affect the behaviour of click
			if constexpr (DoubleClick)
			{
				const time_duration click_duration = s.is_down ? t - s.click_time : time_duration{};
				const auto distance = vector::distance({ m.x_axis, m.y_axis }, s.click_pos);

				//mouse has been clicked previously
				if (click_duration < double_click_time &&
					distance < double_click_distance)
				{
					s.double_clicked = true;
					s.clicked = false;
				}
				else
				{
					s.double_clicked.click_time = t;
					do_click_logic();
				}
			}
			else
				do_click_logic();
		}
		else if (m.active && s.is_down)
		{
			//mouse is still down
			//NOTE: mouse was down last frame, so we've issued a click or a double_click
			//disable the click so it can't trigger twice then check to see if the mouse
			//has moved enough to trigger a drag event
			s.clicked = false;
			if constexpr(Drag)
			{
				const auto distance = vector::distance({ m.x_axis, m.y_axis }, s.click_pos);

				//start a drag event or convert a drag starting into
				// a dragging event
				if (distance > drag_distance &&
					!s.drag.dragging)
				{
					s.drag.drag_start = true;
				}
				else if (distance > drag_distance)
				{
					s.drag.drag_start = false;
					s.drag.dragging = true;
				}
			}
		}
		else if (!m.active && s.is_down)
		{
			//mouse has been released
			//NOTE: mouse was down and is now released
			//set the events that are no longer possible to false
			//call an end to dragging if it was running
			s.clicked = false;

			if constexpr(DoubleClick)
				s.double_clicked = false;

			if constexpr (Drag)
			{
				if (s.drag.dragging || s.drag.drag_start)
				{
					s.drag.drag_start = s.drag.dragging = false;
					s.drag.drag_end = true;
				}
			}
		}
		else
		{
			//mouse has been released for two frames
			//NOTE: this is potentially unneeded, but set everything back to false
			// as no mouse button events should be active after being up for more than 1 frame
			//TODO: only drag_end should possibly still be set by this point, 
			//maybe do some tests and remove all but drag_end from the bellow code
			s.clicked = false;
			if constexpr(DoubleClick)
				s.double_clicked.double_clicked = false;
			if constexpr (Drag)
			{
				s.drag.drag_start = false;
				s.drag.dragging = false;
				s.drag.drag_end = false;
			}
		}

		//update is_down for next frame
		s.is_down = m.active;
	}

	template<bool Drag, bool DoubleClick>
	inline bool is_click(const mouse_button_state<Drag, DoubleClick> &s)
	{
		return s.clicked;
	}

	template<bool Drag, bool DoubleClick>
	inline bool is_double_click(const mouse_button_state<Drag, DoubleClick>&)
	{
		static_assert(DoubleClick, "Double click is disabled for this mouse state");
		return false;
	}

	template<bool Drag>
	inline bool is_double_click(const mouse_button_state<Drag, true> &s)
	{
		return s.double_clicked.double_clicked;
	}

	template<bool Drag, bool DoubleClick>
	inline bool is_drag_start(const mouse_button_state<Drag, DoubleClick>&)
	{
		static_assert(Drag, "Dragging is disabled for this mouse state");
		return false;
	}

	template<bool Drag, bool DoubleClick>
	inline bool is_dragging(const mouse_button_state<Drag, DoubleClick>&)
	{
		static_assert(Drag, "Dragging is disabled for this mouse state");
		return false;
	}

	template<bool Drag, bool DoubleClick>
	inline bool is_drag_end(const mouse_button_state<Drag, DoubleClick>&)
	{
		static_assert(Drag, "Dragging is disabled for this mouse state");
		return false;
	}

	template<bool DoubleClick>
	inline bool is_drag_start(const mouse_button_state<true, DoubleClick> &s)
	{
		return s.drag.drag_start;
	}

	template<bool DoubleClick>
	inline bool is_dragging(const mouse_button_state<true, DoubleClick> &s)
	{
		return s.drag.dragging;
	}

	template<bool DoubleClick>
	inline bool is_drag_end(const mouse_button_state<true, DoubleClick> &s)
	{
		return s.drag.drag_end;
	}
}