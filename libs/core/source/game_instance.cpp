#include "hades/game_instance.hpp"

#include <cassert>

#include "hades/data.hpp"
#include "hades/game_system.hpp"
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
		//TODO: this function pattern is used here and in render_instance,
		//		would it be possible to bring it out into a template?
		assert(_jobs.ready());
		const auto on_create_parent = _jobs.create();
		const auto new_systems = _game.get_new_systems();

		std::vector<job*> jobs;
		for (const auto s : new_systems)
		{
			const auto j = _jobs.create_child(on_create_parent, s->on_create, system_job_data{
				 bad_entity, &_game, nullptr, _current_time, dt, _game.get_system_data(s->id)
				});

			jobs.emplace_back(j);
		}

		const auto systems = _game.get_systems();

		//call on_connect for new entities
		const auto on_connect_parent = _jobs.create();
		for (const auto &s : systems)
		{
			const auto ents = get_added_entites(s.attached_entities, _current_time, _current_time + dt);
			auto& sys_data = _game.get_system_data(s.system->id);

			for (const auto e : ents)
			{
				const auto j = _jobs.create_child_rchild(on_connect_parent, on_create_parent, s.system->on_connect,
					system_job_data{ e, &_game, nullptr, _current_time, dt, sys_data });

				jobs.emplace_back(j);
			}
		}

		//call on_disconnect for removed entities
		const auto on_disconnect_parent = _jobs.create();
		for (const auto &s : systems)
		{
			const auto ents = get_removed_entites(s.attached_entities, _current_time, _current_time + dt);
			auto& sys_data = _game.get_system_data(s.system->id);

			for (const auto e : ents)
			{
				const auto j = _jobs.create_child_rchild(on_disconnect_parent, on_connect_parent, s.system->on_disconnect,
					system_job_data{ e, &_game, nullptr, _current_time, dt, sys_data });

				jobs.emplace_back(j);
			}
		}

		//call on_tick for systems
		const auto on_tick_parent = _jobs.create();
		for (const auto &s : systems)
		{
			const auto entities_curve = s.attached_entities.get();
			const auto ents = entities_curve.get(_current_time);

			auto& sys_data = _game.get_system_data(s.system->id);

			for (const auto e : ents)
			{
				const auto j = _jobs.create_child_rchild(on_tick_parent, on_disconnect_parent, s.system->tick,
					system_job_data{ e, &_game, nullptr, _current_time, dt, sys_data });

				jobs.emplace_back(j);
			}
		}

		_jobs.run(std::begin(jobs), std::end(jobs));
		_jobs.wait(on_tick_parent);
		_jobs.clear();

		_game.clear_new_systems();
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