#ifndef HADES_GAMESYSTEM_HPP
#define HADES_GAMESYSTEM_HPP

#include <functional>
#include <vector>

#include "Hades/GameInterface.hpp"
#include "Hades/parallel_jobs.hpp"
#include "Hades/simple_resources.hpp"

namespace hades
{
	struct system_job_data : public parallel_jobs::job_data
	{
		EntityId entity;
		GameInterface* game_data;
	};

	//the interface for game systems.
	//systems work by creating jobs and passing along the data they will use.
	struct GameSystem
	{
		//this holds the systems, name and id, and the function that the system uses.
		resources::system* system;
		//list of entities attached to this system, over time
		GameInterface::GameCurve<std::vector<EntityId>> attached_entities;
	};
}

#endif //HADES_GAMESYSTEM_HPP