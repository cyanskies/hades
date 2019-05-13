#include "hades/game_instance.hpp"

#include <cassert>

#include "hades/data.hpp"
#include "hades/parallel_jobs.hpp"
#include "hades/core_resources.hpp"
#include "hades/sf_time.hpp"

namespace hades 
{
	game_instance::game_instance(level_save sv) : _game(sv)
	{
	}

	void game_instance::tick(time_duration dt)
	{
		//take a copy of the current active systems and entity attachments, 
		//changes to this wont take effect untill the next tick
		const auto systems = _game.get_systems();

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
					&_game,
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

	void game_instance::add_input(input_system::action_set input, time_point t)
	{
		_game.add_input(std::move(input), t);
	}

	template<typename T>
	static std::vector<exported_curves::export_set<T>> get_exported_set(time_point t,
		typename shared_map<std::pair<entity_id, variable_id>, curve<T>>::data_array data)
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
			auto lower = std::lower_bound(c.second.begin(), c.second.end(), keyframe<T>{t, T{}}, keyframe_less<T>);
			while (lower != c.second.end())
				s.frames.push_back(*lower++);

			if(std::size(s.frames) > 0)
				output.push_back(s);
		}

		return output;
	}

	exported_curves game_instance::get_changes(time_point t) const
	{
		//return all frames between t and time.max	
		const auto &curves = _game.get_curves();

		exported_curves output;

		using namespace resources::curve_types;
		//load all the frames from the specified time into the exported data
		//TODO: half of the curve types are missing
		output.int_curves = get_exported_set<int_t>(t, curves.int_curves.data());
		output.float_curves = get_exported_set<float_t>(t, curves.float_curves.data());
		output.bool_curves = get_exported_set<bool_t>(t, curves.bool_curves.data());
		output.string_curves = get_exported_set<string>(t, curves.string_curves.data());
		output.object_ref_curves = get_exported_set<object_ref>(t, curves.object_ref_curves.data());
		output.unique_curves = get_exported_set<unique>(t, curves.unique_curves.data());

		output.int_vector_curves = get_exported_set<resources::curve_types::vector_int>(t, curves.int_vector_curves.data());

		//add in entityNames
		//output.entity_names = _newEntityNames;
		
		return output;
	}
}