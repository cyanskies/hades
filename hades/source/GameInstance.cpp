#include "Hades/GameInstance.hpp"

#include <cassert>

#include "Hades/Data.hpp"
#include "Hades/parallel_jobs.hpp"
#include "Hades/simple_resources.hpp"

namespace hades 
{
	GameInstance::GameInstance(level_save sv) : GameInterface(sv), _input(curve_type::step)
	{
		//TODO: store input history in sv file and restore it on load
	}

	void GameInstance::tick(sf::Time dt)
	{
		//take a copy of the current active systems and entity attachments, 
		//changes to this wont take effect untill the next tick
		const auto systems = [this] {
			const auto lock = std::lock_guard{ _system_list_mut };
			return _systems;
		} ();

		//NOTE: frame multithreading begins
		// anything that isn't atomic or protected by a lock
		// must be read only

		const auto parent_job = _jobs.create();

		//create jobs for all systems to work on their entities
		for(const auto &s : systems)
		{
			if (!s.system->tick)
				continue;

			assert(s.system);
			const auto entities = s.attached_entities.get().get(_currentTime);

			for (const auto ent : entities)
			{
				const auto j = _jobs.create_child(parent_job, s.system->tick, system_job_data{
					ent,
					s.system->id,
					this,
					nullptr, //mission data //TODO:
					_currentTime,
					dt
				});

				_jobs.run(j);
			}
		}

		_jobs.wait(parent_job);

		_jobs.clear();

		//NOTE: frame multithreading ends

		//advance the game clock
		//this is the only time the clock can be changed
		_currentTime += dt;
	}

	void GameInstance::insertInput(input_system::action_set input, sf::Time t)
	{
		_input.set(t, input);
	}

	template<typename T>
	std::vector<ExportedCurves::ExportSet<T>> GetExportedSet(sf::Time t, typename shared_map<std::pair<EntityId, VariableId>, \
		curve<sf::Time, T>>::data_array data)
	{
		std::vector<ExportedCurves::ExportSet<T>> output;
		for (auto c : data)
		{
			ExportedCurves::ExportSet<T> s;
			s.variable = c.first.second;
			
			auto curve = data::get<resources::curve>(s.variable);
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

		//load all the frames from the specified time into the exported data
		output.intCurves = GetExportedSet<types::int32>(startTime, curves.intCurves.data());
		output.boolCurves = GetExportedSet<bool>(startTime, curves.boolCurves.data());
		output.stringCurves = GetExportedSet<types::string>(startTime, curves.stringCurves.data());
		output.intVectorCurves = GetExportedSet<std::vector<types::int32>>(startTime, curves.intVectorCurves.data());

		//add in entityNames and variable Id mappings
		output.entity_names = _newEntityNames;
		
		return output;
	}

	void GameInstance::nameEntity(EntityId entity, const types::string &name, sf::Time t)
	{
		assert(entity < _next);

		while (true) //NOTE: not a fan of while(true), is there another way to write this without repeating code?
		{
			const auto ent_names = _entity_names.get();
			auto updated_names = ent_names;
			//get the closest list of names to the current time
			auto name_map = updated_names.get(t);

			//if entities can be renamed
			name_map[name] = entity;

			//if entities can not be renamed
			//if (name_map.find(name) != std::end(name_map))
			//	name_map[name] = entity;
			//else
			//	;//throw?

			//insert the new name map back into the curve
			updated_names.insert(t, name_map);

			//replace the shared name list with the updated one
			if (_entity_names.compare_exchange(ent_names, updated_names))
				break; //if we were successful, then break, otherwise run the loop again.
		}
	}
}