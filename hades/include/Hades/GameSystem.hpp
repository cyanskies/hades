#ifndef HADES_GAMESYSTEM_HPP
#define HADES_GAMESYSTEM_HPP

#include <functional>
#include <vector>

#include "Hades/GameInterface.hpp"
#include "Hades/parallel_jobs.hpp"

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
		//this is the function that will be passed to the job system
		//it must do the actual work, or create it's own jobs with other functions
		parallel_jobs::job_function function;
		//list of entities attached to this system, over time
		GameInterface::GameCurve<std::vector<EntityId>> attached_entities;
	};
}

#endif //HADES_GAMESYSTEM_HPP