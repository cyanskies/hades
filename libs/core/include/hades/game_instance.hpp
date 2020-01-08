#ifndef HADES_GAMEINSTANCE_HPP
#define HADES_GAMEINSTANCE_HPP

#include "hades/export_curves.hpp"
#include "hades/game_system.hpp"
#include "hades/level_interface.hpp"
#include "hades/level.hpp"
#include "hades/timers.hpp"

namespace hades
{
	class variable_without_name : public std::logic_error
	{
		using std::logic_error::logic_error;
	};

	//represents the logic side of an individual level or game
	//TODO:
	// support saving
	class game_instance
	{
	public:
		game_instance(level_save);

		//triggers all systems with the specified time change
		void tick(time_duration dt);

		void add_input(std::vector<server_action> input, time_point t);

		//exports all the newest keyframes so that they can be transmitted across the network
		//also sends entity name mappings
		//  t: send all keyframes after this point
		//		default is max time
		void get_changes(exported_curves&, time_point t) const;

		time_point get_time(time_point mission_offset = time_point{}) const noexcept;

		const game_interface* get_interface() const noexcept;

	private:
		//the games starting time, is always 0
		constexpr static time_point _start_time{};
		
		time_point _prev_time{};
		//the current time, this is the sum of all dt from the game ticks
		time_point _current_time{};

		game_implementation _game;
		//each tick will generate events that are handled at the end of that tick
		//events created by other events will be handled at the end of the next tick
	};
}

#endif //HADES_GAMEINSTANCE_HPP
