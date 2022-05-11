#ifndef HADES_ACTION_INPUT_HPP
#define HADES_ACTION_INPUT_HPP

#include "hades/input.hpp"
#include "hades/time.hpp"

namespace hades
{
	namespace detail
	{
		struct action_extra
		{
			time_point last_active = {};
		};

		struct action_double
		{
			bool double_tapped = false;
		};

		struct action_held
		{
			bool held = false;
		};

		struct action_empty {};
		struct action_empty2 {};
		struct action_empty3 {};
	}

	//can be used to track individual presses.
	// double taps
	// and holding the button down.
	template<bool DoubleTap = true, bool Holdable = true>
	struct action_state : public std::conditional_t<DoubleTap || Holdable, detail::action_extra, detail::action_empty>,
		public std::conditional_t<DoubleTap, detail::action_double, detail::action_empty2>,
		public std::conditional_t<Holdable, detail::action_held, detail::action_empty3>
	{
		bool pressed = false;
	};

	//functions for testing the state
	template<bool DoubleTap, bool Holdable>
	constexpr void update_action_state(const action& a, const time_point& time, action_state<DoubleTap, Holdable>& state) noexcept;
	template<bool DoubleTap, bool Holdable>
	constexpr bool action_pressed(const action_state<DoubleTap, Holdable>& state) noexcept; // returns true if the button was just pressed
	template<bool DoubleTap, bool Holdable>
	constexpr bool action_down(const action_state<DoubleTap, Holdable>& state) noexcept; // returns true as long as the button is down; pressed || held
	template<bool DoubleTap, bool Holdable>
	constexpr bool action_double_tap(const action_state<DoubleTap, Holdable>& state) noexcept; // returns true if the button has been double tapped
	template<bool DoubleTap, bool Holdable>
	constexpr bool action_held(const action_state<DoubleTap, Holdable>& state) noexcept; // returns true if the button has been held down
	template<bool DoubleTap, bool Holdable>
	constexpr time_point action_hold_start_time(const action_state<DoubleTap, Holdable>& state) noexcept;

}

#include "hades/detail/action_input.inl"

#endif // HADES_ACTION_INPUT_HPP
