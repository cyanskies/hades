#ifndef HADES_GAMEINSTANCE_HPP
#define HADES_GAMEINSTANCE_HPP

#include "hades/export_curves.hpp"
#include "hades/game_system.hpp"
#include "hades/level_interface.hpp"
#include "hades/level.hpp"
#include "hades/parallel_jobs.hpp"
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
	class game_instance : public game_interface
	{
	public:
		game_instance(level_save);

		//triggers all systems with the specified time change
		void tick(time_duration dt);

		void insertInput(input_system::action_set input, time_point t);

		//exports all the newest keyframes so that they can be transmitted across the network
		//also sends the new variable id mappings
		//also sends entity name mappings
		//  t: send all keyframes after this point
		//		default is max time
		exported_curves getChanges(time_point t) const;
		//clears the new entity name list so the same list of entities don't get sent every tick
		//names are used for common entities to make finding them easy
		// master: the games master entity, used to store game rules, whether the game as ended yet
		// terrain: the terrain entity, all the world terrain will be stored as this entity's properties
		void nameEntity(entity_id entity, const types::string &name, time_point t);

	private:
		//the games starting time, is always 0
		constexpr static time_point _start_time{};
		//the current time, this is the sum of all dt from the game ticks
		time_point _current_time{};

		//diffs for the entitynames list
		std::vector<std::pair<entity_id, types::string>> _newEntityNames;

		curve<input_system::action_set> _input;
		//each tick will generate events that are handled at the end of that tick
		//events created by other events will be handled at the end of the next tick

		job_system _jobs;
	};
}

#endif //HADES_GAMEINSTANCE_HPP