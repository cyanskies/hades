#include "hades/game_instance.hpp"

#include <cassert>

#include "hades/data.hpp"
#include "hades/game_system.hpp"
#include "hades/parallel_jobs.hpp"
#include "hades/core_resources.hpp"
#include "hades/sf_time.hpp"

namespace hades 
{
	game_instance::game_instance(level_save sv) : _game(sv), 
		_jobs{*console::get_int(cvars::server_threadcount, cvars::default_value::server_threadcount)}
	{
	}

	void game_instance::tick(time_duration dt)
	{
		auto make_game_struct = [](entity_id e, game_interface* g, time_point t, time_duration dt, system_data_t* d)->system_job_data {
			return system_job_data{ e, g, nullptr, t, dt, d };
		};

		const auto next = update_level(_jobs, _prev_time, _current_time, dt, _game, make_game_struct);
		_prev_time = _current_time;
		_current_time = next;
		return;
	}

	void game_instance::add_input(input_system::action_set input, time_point t)
	{
		_game.add_input(std::move(input), t);
	}

	template<typename T>
	static std::vector<exported_curves::export_set<T>> get_exported_set(time_point t,
		const typename shared_map<std::pair<entity_id, variable_id>, curve<T>>::data_array &data)
	{
		std::vector<exported_curves::export_set<T>> output;
		for (const auto &c : data)
		{
			exported_curves::export_set<T> s;
			s.variable = c.first.second;
			//TODO: cache this somewhere so wi don't hit a mutex every loop
			//auto curve = data::get<resources::curve>(s.variable);
			//assert(curve);
			////skip if this curve shouldn't sync to the client
			//if (!curve->sync)
			//	continue;

			//get the rest of the data
			s.entity = c.first.first;
			auto lower = std::lower_bound(c.second.begin(), c.second.end(), keyframe<T>{t, T{}}, keyframe_less<T>);
			while (lower != c.second.end())
				s.frames.emplace_back(*lower++);

			if(std::size(s.frames) > 0)
				output.emplace_back(std::move(s));
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
		output.int_curves = get_exported_set<int_t>(t, curves.int_curves.data_no_async());
		output.float_curves = get_exported_set<float_t>(t, curves.float_curves.data_no_async());
		output.bool_curves = get_exported_set<bool_t>(t, curves.bool_curves.data_no_async());
		output.string_curves = get_exported_set<string>(t, curves.string_curves.data_no_async());
		output.object_ref_curves = get_exported_set<object_ref>(t, curves.object_ref_curves.data_no_async());
		output.unique_curves = get_exported_set<unique>(t, curves.unique_curves.data_no_async());

		output.int_vector_curves = get_exported_set<resources::curve_types::vector_int>(t, curves.int_vector_curves.data_no_async());

		//add in entityNames
		//output.entity_names = _newEntityNames;
		
		return output;
	}
}