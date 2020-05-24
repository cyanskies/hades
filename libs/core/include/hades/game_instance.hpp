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

	struct player_data;

	//wraps the logic of a level with timekeeping
	class game_instance
	{
	public:
		game_instance(const level_save&);

		//triggers all systems with the specified time change
		void tick(time_duration dt, const std::vector<player_data>*);

		void add_input(unique_id, std::vector<action> input, time_point t);

		//exports all the newest keyframes so that they can be transmitted across the network
		//also sends entity name mappings
		//  t: send all keyframes after this point
		//		default is max time
		void get_changes(exported_curves&, time_point t) const;
		time_point get_time(time_point mission_offset = time_point{}) const noexcept;
		game_interface* get_interface() noexcept;

	private:
		//the games starting time, is always 0
		constexpr static time_point _start_time{};
		
		time_point _prev_time{};
		//the current time, this is the sum of all dt from the game ticks
		time_point _current_time{};

		game_implementation _game;
	};
}

#endif //HADES_GAMEINSTANCE_HPP
