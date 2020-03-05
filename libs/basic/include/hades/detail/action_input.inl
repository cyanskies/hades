namespace hades
{
	constexpr time_duration double_tap_time = time_cast<time_duration>(seconds_float{ 0.30f });

	template<bool DoubleTap, bool Holdable>
	constexpr inline void update_action_state(const action& a, const time_point& t, action_state<DoubleTap, Holdable>& s) noexcept
	{
		//check each combination of action
		//action is down / not down
		//action was down / wans't down last frame

		//test for action being activated or double tapped
		if (a.active && !s.pressed)
		{
			if constexpr (DoubleTap)
			{
				assert(t > s.last_active);
				const time_duration active_duration = t - s.last_active;

				//mouse has been clicked previously
				if (active_duration < double_tap_time)
				{
					s.double_tapped = true;
					s.last_active = {};
				}
				else
				{
					s.last_active = t;
					s.pressed = true;
				}
			}
			else
				s.pressed = true;
		}
		if (a.active && s.pressed)
		{
			s.pressed = false;
			if constexpr (Holdable)
			{
				if (t != s.last_active)
					s.held = true;
			}
		}
		else if (!a.active)
		{
			s.pressed = false;
			if constexpr (Holdable)
				s.held = false;
			if constexpr (DoubleTap)
				s.double_tapped = false;
		}
	}

	template<bool DoubleTap, bool Holdable>
	constexpr bool action_pressed(const action_state<DoubleTap, Holdable>& state) noexcept
	{
		return state.pressed;
	}

	template<bool DoubleTap, bool Holdable>
	constexpr bool action_down(const action_state<DoubleTap, Holdable>& state) noexcept
	{
		if constexpr (Holdable)
		{
			return state.held || state.pressed;
		}
		else
			return state.pressed;
	}

	template<bool DoubleTap, bool Holdable>
	constexpr bool action_double_tap(const action_state<DoubleTap, Holdable>& state) noexcept
	{
		static_assert(DoubleTap, "Double tap is disabled for this mouse state");
		return false;
	}

	template<bool Holdable>
	constexpr bool action_double_tap(const action_state<true, Holdable>& state) noexcept
	{
		return state.double_tap;
	}

	template<bool DoubleTap, bool Holdable>
	constexpr bool action_held(const action_state<DoubleTap, Holdable>& state) noexcept
	{
		static_assert(Holdable, "holding is disabled for this mouse state");
		return false;
	}

	template<bool DoubleTap>
	constexpr bool action_held(const action_state<DoubleTap, true>& state) noexcept
	{
		return state.held;
	}

	template<bool DoubleTap, bool Holdable>
	constexpr time_point action_hold_start_time(const action_state<DoubleTap, Holdable>& state) noexcept
	{
		static_assert(Holdable, "holding is disabled for this mouse state");
		return false;
	}

	template<bool DoubleTap>
	constexpr bool action_hold_start_time(const action_state<DoubleTap, true>& state) noexcept
	{
		return state.last_active;
	}
}
