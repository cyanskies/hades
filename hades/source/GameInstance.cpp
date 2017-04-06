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

		//create jobs for all systems on all attached
		for(auto &s : _systems)
		{
			//create the job data, with a pointer to the game interface
			auto data = std::make_unique<system_job_data>();
			data->game_data = this;

			auto ents = s.attached_entities.get().get(_currentTime);

			if (!s.system)
			{
				//empty system
				//TODO: error
				assert(s.system);
			}

			//create a job function that will call the systems job on each attached entity
			*iter = parallel_jobs::create([ents, func = s.system->value](const parallel_jobs::job_data &data) { 
				std::vector<parallel_jobs::job*> jobs(ents.size());
				auto funciter = jobs.begin();

				for (auto e : ents)
				{
					auto job_data = std::make_unique<system_job_data>();
					job_data->entity = e;
					auto d = static_cast<const system_job_data*>(&data);
					job_data->game_data = d->game_data;
					*funciter = parallel_jobs::create(func, std::move(job_data));
					++funciter;
				}

				for (auto j : jobs)
					parallel_jobs::run(j);

				for (auto j : jobs)
					parallel_jobs::wait(j);

				return true;
			}, std::move(data));

			++iter;
		}

		//the jobs must be started after they are all created,
		//in case any systems try to add or remove entities from other systems
		for (auto j : jobs)
			parallel_jobs::run(j);

		for (auto j : jobs)
			parallel_jobs::wait(j);

		//advance the game clock
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
		std::lock_guard<std::shared_mutex> lk(_entNameMutex);
		_entityNames[name] = entity;
	}

	types::string GameInstance::getVariableName(VariableId id) const
	{
		std::shared_lock<std::shared_mutex> lk(_variableIdMutex);
		for (auto n : _variableIds)
		{
			if (n.second == id)
				return n.first;
		}

		//TODO: throw instead;
		return types::string();
	}
}