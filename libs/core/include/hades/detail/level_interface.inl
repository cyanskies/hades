#include "hades/level_interface.hpp"

#include "hades/properties.hpp"
#include "hades/console_variables.hpp"
#include "hades/players.hpp"

namespace hades
{
	namespace detail
	{
		inline void set_data(system_job_data* d) noexcept
		{
			return set_game_data(d);
		}

		inline void set_data(render_job_data* d) noexcept
		{
			return set_render_data(d);
		}
	}

	template<typename Func, typename JobDataType, typename Interface>
	constexpr auto make_game_struct_is_invokable = std::is_nothrow_invocable_r_v<JobDataType,
		Func, JobDataType, Interface*, Interface*, time_duration, const std::vector<player_data>*>;

	template<typename Interface, typename JobDataType, typename MakeGameStructFn>
	void update_systems_first_frame(JobDataType jdata, time_duration dt,
		Interface& interface, Interface* mission, const std::vector<player_data>* players, MakeGameStructFn&& make_game_struct)
	{

		using SystemType = typename JobDataType::system_type;
		auto& sys_behaviours = *jdata.systems;

		struct on_connect_data
		{
			SystemType* system;
			name_list ents;
		};

		auto on_connect_data_vec = std::vector<on_connect_data>{};

		const auto new_systems = sys_behaviours.get_new_systems();
		const auto systems = sys_behaviours.get_systems();
		for (const auto s : new_systems)
		{
			SystemType* system = nullptr;
			for (auto& sys : systems)
			{
				if (sys->system == s)
				{
					system = sys;
					break;
				}
			}
			assert(system);

			//pass entities that are already attached to this system
			//this will be entities that were already in the level file
			//or save file before this time point
			auto current_ents = sys_behaviours.get_created_entities(*system);

			on_connect_data_vec.emplace_back(on_connect_data{ system, std::move(current_ents) });

			if (!s->on_create)
				continue;

			auto game_data = std::invoke(make_game_struct, jdata, &interface, mission, dt, players);
			game_data.entity = activated_object_view{ current_ents, time_point::min() };
			game_data.system = s->id;
			game_data.system_data = &sys_behaviours.get_system_data(s->id);

			detail::set_data(&game_data);
			std::invoke(s->on_create);
		} // for (new_systems) on_create

		for (auto& sys : on_connect_data_vec)
		{
			auto system = sys.system->system;

			if (!system->on_connect)
				continue;

			auto game_data = std::invoke(make_game_struct, jdata, &interface, mission, dt, players);
			game_data.entity = activated_object_view{ sys.ents, time_point::min() };
			game_data.system = system->id;
			game_data.system_data = &sys_behaviours.get_system_data(game_data.system);

			detail::set_data(&game_data);
			std::invoke(system->on_connect);
		}

		return;
	}

	// this is called before and after level update. this ensures that after a game tick, all objects have
	// had their appropriate on_connect/disconnect functions called
	// we dont need to worry about them when loading a save game
	template<typename Interface, typename JobDataType, typename MakeGameStructFn>
	void update_systems(JobDataType jdata, time_duration dt,
		Interface& interface, Interface* mission, const std::vector<player_data>* players, MakeGameStructFn&& make_game_struct)
	{
		using job_data_type = JobDataType;
		static_assert(make_game_struct_is_invokable<MakeGameStructFn, JobDataType, Interface>,
			"make_game_struct must return the correct job_data_type");

		using SystemType = typename JobDataType::system_type;

		auto& sys_behaviours = *jdata.systems;

		while (sys_behaviours.needs_update())
		{
			const auto new_systems = sys_behaviours.get_new_systems();
			const auto systems = sys_behaviours.get_systems();
			for (const auto s : new_systems)
			{
				if (!s->on_create)
					continue;

				SystemType* system = nullptr;
				for (auto sys : systems)
				{
					if (sys->system == s)
					{
						system = sys;
						break;
					}
				}
				assert(system);

				//pass entities that are already attached to this system
				//this will be entities that were already in the level file
				//or save file before this time point
				auto current_ents = sys_behaviours.get_created_entities(*system);

				auto game_data = std::invoke(make_game_struct, jdata, &interface, mission, dt, players);
				game_data.entity = { current_ents, time_point::min() };
				game_data.system = s->id;
				game_data.system_data = &sys_behaviours.get_system_data(s->id);

				detail::set_data(&game_data);
				std::invoke(s->on_create);
			} // for (new_systems) on_create

			// on connect
			for (auto& s : systems)
			{
				auto ents = sys_behaviours.get_new_entities(*s);

				if (s->system->on_connect && !std::empty(ents))
				{
					auto& sys_data = sys_behaviours.get_system_data(s->system->id);
					auto game_data = std::invoke(make_game_struct, jdata, &interface, nullptr, dt, players);
					game_data.entity = activated_object_view{ ents, time_point::min() };
					game_data.system = s->system->id;
					game_data.system_data = &sys_data;

					detail::set_data(&game_data);
					std::invoke(s->system->on_connect);
				}
			}//on connect

			// on disconnect
			for (auto s : systems)
			{
				auto ents = sys_behaviours.get_removed_entities(*s);

				if (s->system->on_disconnect && !std::empty(ents))
				{
					auto& sys_data = sys_behaviours.get_system_data(s->system->id);
					auto game_data = std::invoke(make_game_struct, jdata, &interface, nullptr, dt, players);
					game_data.entity = activated_object_view{ ents, time_point::min() };
					game_data.system = s->system->id;
					game_data.system_data = &sys_data;

					detail::set_data(&game_data);
					std::invoke(s->system->on_disconnect);
				}
			}//on disconnect
		}//while(needs update)
		return;
	}

	template<typename Interface, typename JobDataType, typename MakeGameStructFn>
	time_point update_level(JobDataType job_data, time_duration dt,
		Interface& interface, Interface* mission, const std::vector<player_data>* players, MakeGameStructFn&& make_game_struct)
	{
		using job_data_type = JobDataType;
		static_assert(make_game_struct_is_invokable<MakeGameStructFn, JobDataType, Interface>,
			"make_game_struct must return the correct job_data_type");

		if constexpr (std::is_same_v<JobDataType, system_job_data>)
		{
			if (job_data.current_time == time_point{})
				update_systems_first_frame(job_data, dt, interface, mission, players, make_game_struct);
		}

		using SystemType = typename JobDataType::system_type;

		const auto current_time = job_data.current_time + dt;
		
		// when a level is first loaded on_create needs to be called for all systems
		// otherwise, on_connect/on_create should be called at the end of the tick
		// on which they were queued

		// if the systems data is dirty then call on_create
		//and on_connect as needed
		update_systems(job_data, dt, interface, mission, players, make_game_struct);

		//input functions
		//NOTE: input is only triggered on the server
		//TODO: input queue per player slot,
		if constexpr (std::is_same_v<std::decay_t<Interface>, game_implementation>)
		{
			const auto player_input_fn = interface.get_player_input_function();
			if (player_input_fn)
			{
				auto input_q = interface.get_and_clear_input_queue();
				//player input function, no system data available
				using ent_list = resources::curve_types::collection_object_ref;
				auto game_data = std::invoke(make_game_struct, job_data, &interface, mission, dt, players);
				for (auto p : *players)
				{
					using state = player_data::state;
					if (p.player_state == state::empty)
						continue;

					auto iter = input_q.find(p.name);
					if (iter != end(input_q))
					{
						detail::set_data(&game_data);
						std::invoke(player_input_fn, p, std::move(iter->second), current_time);
					}
				}

				// player func may have created/destroyed ents
				update_systems(job_data, dt, interface, mission, players, make_game_struct);
			}
		}

		auto& sys_behaviours = *job_data.systems;
		const auto systems = sys_behaviours.get_systems();

		//call on_tick for systems
		for (auto* s : systems)
		{
			if (!s->system->tick)
				continue;

			// TODO: FIXME: this is a reference to the current connected entities list
			// entities that are created or destroyed during this tick
			// will be added, removed from this list, while we are
			// still iterating over it. = bug
			const auto& current_ents = sys_behaviours.get_entities(*s);
			//const auto current_ents = sys_behaviours.get_entities(*s);

			auto& sys_data = sys_behaviours.get_system_data(s->system->id);
			auto game_data = std::invoke(make_game_struct, job_data, &interface, mission, dt, players);
			game_data.entity = activated_object_view{ current_ents, current_time };
			game_data.system = s->system->id;
			game_data.system_data = &sys_data;

			detail::set_data(&game_data);
			std::invoke(s->system->tick);
		}

		//update systems again, to ensure that everything has been properly called,
		//before a possible save
		update_systems(job_data, dt, interface, mission, players, make_game_struct);
		return current_time;
	}
}
