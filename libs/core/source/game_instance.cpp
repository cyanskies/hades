#include "hades/game_instance.hpp"

#include <cassert>

#include "hades/data.hpp"
#include "hades/parallel_jobs.hpp"
#include "hades/core_resources.hpp"
#include "hades/time.hpp"

namespace hades 
{
	GameInstance::GameInstance(level_save sv) : game_interface(sv), _input(curve_type::step)
	{
		//TODO: store input history in sv file and restore it on load
	}

	void GameInstance::tick(time_duration dt)
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
			const auto entities = s.attached_entities.get().get(_current_time);

			for (const auto ent : entities)
			{
				const auto j = _jobs.create_child(parent_job, s.system->tick, system_job_data{
					ent,
					s.system->id,
					this,
					nullptr, //mission data //TODO:
					_current_time,
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
		_current_time += dt;
	}

	void GameInstance::insertInput(input_system::action_set input, time_point t)
	{
		_input.set(t, input);
	}

	template<typename T>
	std::vector<exported_curves::export_set<T>> GetExportedSet(time_point t, typename shared_map<std::pair<entity_id, variable_id>, \
		curve<T>>::data_array data)
	{
		std::vector<exported_curves::export_set<T>> output;
		for (auto c : data)
		{
			exported_curves::export_set<T> s;
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

	exported_curves GameInstance::getChanges(time_point t) const
	{
		//return all frames between t and time.max	
		const auto &curves = get_curves();

		exported_curves output;

		using namespace resources::curve_types;
		//load all the frames from the specified time into the exported data
		//TODO: half of the curve types are missing
		output.int_curves = GetExportedSet<int_t>(t, curves.int_curves.data());
		output.bool_curves = GetExportedSet<bool_t>(t, curves.bool_curves.data());
		output.string_curves = GetExportedSet<string>(t, curves.string_curves.data());
		output.int_vector_curves = GetExportedSet<resources::curve_types::vector_int>(t, curves.int_vector_curves.data());

		//add in entityNames and variable Id mappings
		output.entity_names = _newEntityNames;
		
		return output;
	}

	void GameInstance::nameEntity(entity_id entity, const types::string &name, time_point time)
	{
		assert(entity < entity_id{ _next });

		while (true) //NOTE: not a fan of while(true), is there another way to write this without repeating code?
		{
			const auto ent_names = _entity_names.get();
			auto updated_names = ent_names;
			//get the closest list of names to the current time
			auto name_map = updated_names.get(time);

			//if entities can be renamed
			name_map[name] = entity;

			//if entities can not be renamed
			//if (name_map.find(name) != std::end(name_map))
			//	name_map[name] = entity;
			//else
			//	;//throw?

			//insert the new name map back into the curve
			updated_names.insert(time, name_map);

			//replace the shared name list with the updated one
			if (_entity_names.compare_exchange(ent_names, updated_names))
				break; //if we were successful, then break, otherwise run the loop again.
		}
	}
}