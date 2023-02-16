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

	// this is called before and after level update. this ensures that after a game tick, all objects have
	// had their appropriate on_connect/disconnect functions called
	// we dont need to worry about them when loading a save game
	template<typename JobDataType>
	void update_systems(JobDataType jdata)
	{
		using SystemType = typename JobDataType::system_type;

		assert(jdata.systems);
		auto& sys_behaviours = *jdata.systems;

		while (sys_behaviours.needs_update())
		{
			const auto new_systems = sys_behaviours.get_new_systems();
			const auto systems = sys_behaviours.get_systems();
			for (const auto s : new_systems)
			{
				assert(s);
				if (!s->on_create)
					continue;

				SystemType* system = nullptr;
				for (auto sys : systems)
				{
					assert(sys);
					if (sys->system == s)
					{
						system = sys;
						break;
					}
				}
				assert(system); // TODO: throw

				//pass entities that are already attached to this system
				//this will be entities that were already in the level file
				//or save file before this time point
				auto current_ents = sys_behaviours.get_created_entities(*system);

				auto game_data = jdata;
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
					auto game_data = jdata;
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
					auto game_data = jdata;
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

	template<typename Interface, typename JobDataType>
	time_point update_level(JobDataType job_data, Interface& interface)
	{
		using SystemType = typename JobDataType::system_type;

		const auto& current_time = job_data.current_time;
		
		// when a level is first loaded on_create needs to be called for all systems
		// otherwise, on_connect/on_create should be called at the end of the tick
		// on which they were queued

		// if the systems data is dirty then call on_create
		//and on_connect as needed
		update_systems(job_data);

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
				auto game_data = job_data;
				for (auto p : *game_data.players)
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
				update_systems(job_data);
			}
		}

		auto& sys_behaviours = *job_data.systems;
		const auto systems = sys_behaviours.get_systems();

		//call on_tick for systems
		for (auto* s : systems)
		{
			if (!s->system->tick)
				continue;

			auto& current_ents = sys_behaviours.get_entities(*s);

			auto& sys_data = sys_behaviours.get_system_data(s->system->id);
			auto game_data = job_data;
			game_data.entity = activated_object_view{ current_ents, current_time };
			game_data.system = s->system->id;
			game_data.system_data = &sys_data;

			detail::set_data(&game_data);
			std::invoke(s->system->tick);
		}

		//update systems again, to ensure that everything has been properly called,
		//before a possible save
		update_systems(job_data);
		return current_time;
	}
}
