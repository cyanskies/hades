#include "Hades/GameInstance.hpp"

#include <cassert>

#include "Hades/parallel_jobs.hpp"

namespace hades 
{
	void GameInstance::tick(sf::Time dt)
	{
		assert(parallel_jobs::ready());

		std::vector<parallel_jobs::job*> jobs(_systems.size());
		auto iter = jobs.begin();

		//create jobs for all systems to work on their entities
		for(auto &s : _systems)
		{
			if (!s.system)
			{
				//empty system
				//TODO: error
				assert(s.system);
			}

			//create the job data, with a pointer to the game interface
			auto job_data = std::make_unique<system_job_data>();
			job_data->entities = s.attached_entities.get().get(_currentTime);
			job_data->game_data = this;
			job_data->current_time = _currentTime;
			job_data->dt = dt;

			//create a job function that will call the systems job on each attached entity
			*iter = parallel_jobs::create(s.system->value, std::move(job_data));
			++iter;
		}

		//the jobs must be started after they are all created,
		//in case any systems try to add or remove entities from other systems
		for (auto j : jobs)
			parallel_jobs::run(j);

		for (auto j : jobs)
			parallel_jobs::wait(j);

		assert(parallel_jobs::ready());

		//advance the game clock
		//this is the only time the clock can be changed
		_currentTime += dt;
	}

	void GameInstance::getChanges(sf::Time t) const
	{
		//return all frames after 't'
	}

	void GameInstance::getAll() const
	{
		//return all frames
	}

	void GameInstance::nameEntity(EntityId entity, const types::string &name)
	{
		std::lock_guard<std::shared_mutex> lk(EntNameMutex);
		EntityNames[name] = entity;
	}

	types::string GameInstance::getVariableName(VariableId id) const
	{
		std::shared_lock<std::shared_mutex> lk(VariableIdMutex);
		for (auto n : VariableIds)
		{
			if (n.second == id)
				return n.first;
		}

		//TODO: throw instead;
		return types::string();
	}
}