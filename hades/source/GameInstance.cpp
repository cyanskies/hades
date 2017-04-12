#include "Hades/GameInstance.hpp"

#include <cassert>

#include "Hades/DataManager.hpp"
#include "Hades/parallel_jobs.hpp"
#include "Hades/simple_resources.hpp"

namespace hades 
{
	void GameInstance::tick(sf::Time dt)
	{
		assert(parallel_jobs::ready());

		std::shared_lock<std::shared_mutex> system_lock(SystemsMutex);

		std::vector<parallel_jobs::job*> jobs(Systems.size());
		auto iter = jobs.begin();

		//create jobs for all systems to work on their entities
		for(auto &s : Systems)
		{
			if (!s.system)
			{
				assert(s.system);
				throw system_null("Tried to run job from system, but the systems pointer was null");
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

	template<typename T>
	std::vector<ExportedCurves::ExportSet<T>> GetExportedSet(sf::Time t, typename transactional_map<std::pair<EntityId, VariableId>, \
		Curve<sf::Time, T>>::data_array data, const std::map<VariableId, data::UniqueId> &variableIds)
	{
		std::vector<ExportedCurves::ExportSet<T>> output;
		for (auto c : data)
		{
			ExportedCurves::ExportSet<T> s;
			s.variable = c.first.second;
			
			//attempt early bailout if this curve isn't for syncing
			auto curve_id = variableIds.find(s.variable);
			assert(curve_id != variableIds.end());
			auto curve = data_manager->getCurve(curve_id->second);
			assert(curve);
			//skip if this curve shouldn't sync to the client
			if (!curve->sync)
				continue;

			//get the rest of the data
			s.entity = c.first.first;
			auto lower = std::lower_bound(c.second.begin(), c.second.end(), t);
			while (lower != c.second.end())
				s.frames.push_back(*lower++);

			output.push_back(s);
		}

		return output;
	}

	ExportedCurves GameInstance::getChanges(sf::Time t) const
	{
		//return all frames between currenttime - t and time.max	
		auto startTime = _currentTime - t;
		auto &curves = getCurves();

		ExportedCurves output;

		std::map<VariableId, data::UniqueId> reverseIds;
		{
			std::shared_lock<std::shared_mutex> lk(VariableIdMutex);
			for (auto i : VariableIds)
				reverseIds.insert({ i.second, i.first });
		}

		//load all the frames from the specified time into the exported data
		output.intCurves = GetExportedSet<types::int32>(startTime, curves.intCurves.data(), reverseIds);
		output.boolCurves = GetExportedSet<bool>(startTime, curves.boolCurves.data(), reverseIds);
		output.stringCurves = GetExportedSet<types::string>(startTime, curves.stringCurves.data(), reverseIds);
		output.intVectorCurves = GetExportedSet<std::vector<types::int32>>(startTime, curves.intVectorCurves.data(), reverseIds);

		//add in entityNames and variable Id mappings
		output.entity_names = _newEntityNames;
		
		return output;
	}

	void GameInstance::clearNewEntitiesNames()
	{
		_newEntityNames.clear();
	}

	std::vector<std::pair<EntityId, types::string>> GameInstance::getAllEntityNames() const
	{
		std::shared_lock<std::shared_mutex> lk(EntNameMutex);
		std::vector<std::pair<EntityId, types::string>> out;

		for (auto n : EntityNames)
			out.push_back({ n.second, n.first });

		return out;
	}

	void GameInstance::nameEntity(EntityId entity, const types::string &name)
	{
		std::lock_guard<std::shared_mutex> lk(EntNameMutex);
		EntityNames[name] = entity;
		_newEntityNames.push_back({ entity, name });
	}

	void GameInstance::registerVariable(data::UniqueId id)
	{
		std::lock_guard<std::shared_mutex> lk(VariableIdMutex);
		VariableIds[id] = ++VariableNext;
	}


	types::string GameInstance::getVariableName(VariableId id) const
	{
		std::shared_lock<std::shared_mutex> lk(VariableIdMutex);
		for (auto n : VariableIds)
		{
			if (n.second == id)
				return data_manager->as_string(n.first);
		}

		throw variable_without_name("Tried to find name for variable id: " + std::to_string(id) + "; but it had no recorded name");
	}

	void GameInstance::installSystem(resources::system *system)
	{
		assert(system);

		std::lock_guard<std::shared_mutex> system_lock(SystemsMutex);

		if (!system)
			throw system_null("system pointer passed to GameInstance::installSystem was null");

		auto id = system->id;

		for (auto s : Systems)
		{
			if (s.system->id == id)
				throw system_already_installed("tried to install system: <name>; that is already installed");
		}

		GameSystem s;
		s.system = system;
		Curve<sf::Time, std::vector<EntityId>> entities(CurveType::STEP);
		entities.insert(sf::Time::Zero, {});
		s.attached_entities = entities;
	}
}