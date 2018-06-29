#ifndef HADES_MISSION_HPP
#define HADES_MISSION_HPP

#include "Hades/GameInterface.hpp"
#include "hades/uniqueid.hpp"

//a mission is a unit of play

//for some games a single mission represents the whole game
//like for a zelda game

//for others a campaign is a collection of missions, like a command and conquer game

namespace hades
{
	//mission structure
	//	mission entities and starting state
	//	list of levels referenced by the mission


	//a mission is started by converting it into a mission save, and then loading it.
	struct mission_info
	{
		curve_data entity_data;
		std::map<types::string, EntityId> entity_names;
	};

	struct mission
	{
		//starting mission entity state
		mission_info mission_data;

		//list of levels.
		std::vector<unique_id> levels;
	};

	struct mission_save
	{
		//ref to origional mission
		types::string source_file;

		//current mission entity state
		mission_info mission_data;

		//list of level saves
	};

	mission_save convert_to_save(mission m);
}

#endif //HADES_MISSION_HPP
