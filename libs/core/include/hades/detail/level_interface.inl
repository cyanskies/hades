#include "hades/level_interface.hpp"

#include "hades/properties.hpp"
#include "hades/console_variables.hpp"

namespace hades
{
	namespace detail
	{
		static inline void set_data(system_job_data* d) noexcept
		{
			return set_game_data(d);
		}

		static inline void set_data(render_job_data* d) noexcept
		{
			return set_render_data(d);
		}
	}

	template<typename Interface, typename SystemType, typename MakeGameStructFn>
	time_point update_level(time_point /*before_prev*/, time_point prev_time, time_duration dt,
		Interface& interface, system_behaviours<SystemType> &sys_behaviours,const std::vector<player_data>* players, MakeGameStructFn make_game_struct)
	{
		using job_data_type = typename SystemType::job_data_t;
		static_assert(std::is_invocable_r_v<job_data_type, MakeGameStructFn, unique_id,
			std::vector<entity_id>, Interface*, system_behaviours<SystemType>*, time_point, time_duration, const std::vector<player_data>*, system_data_t*>,
			"make_game_struct must return the correct job_data_type");

		const auto current_time = prev_time + dt;
		
		{
			const auto new_systems = sys_behaviours.get_new_systems();
			auto& systems = sys_behaviours.get_systems();
			for (const auto s : new_systems)
			{
				if (!s->on_create)
					continue;

				SystemType* system = nullptr;
				for (auto &sys : systems)
				{
					if (sys.system == s)
					{
						system = &sys;
						break;
					}
				}
				assert(system);

				//pass entities that are already attached to this system
				//this will be entities that were already in the level file
				//or save file before this time point
				auto ents = std::vector<entity_id>{};
				const auto current_ents = sys_behaviours.get_entities(*system).get(prev_time);
				ents.reserve(std::size(current_ents));
				std::transform(std::begin(current_ents), std::end(current_ents), std::back_inserter(ents),
					[](auto &&entity) {
						return std::get<entity_id>(entity);
				});

				auto game_data = std::invoke(make_game_struct, s->id, std::move(ents), &interface, &sys_behaviours, prev_time, dt, players, &sys_behaviours.get_system_data(s->id));
				detail::set_data(&game_data);
				std::invoke(s->on_create);
			}
		}

		struct update_data
		{
			const SystemType::system_t* system = nullptr;
			resources::curve_types::collection_object_ref added_ents, attached_ents, removed_ents;
		};

		//collect entity lists upfront, so that they don't change mid update
		auto data = std::vector<update_data>{};
		
		{
			auto &systems = sys_behaviours.get_systems();
			data.reserve(size(systems));

			for (auto& s : systems)
			{
				auto d = update_data{ s.system };
				if (s.system->on_connect)
					d.added_ents = sys_behaviours.get_new_entities(s);

				if (s.system->tick)
				{
					//only update entities that have passed their wake up time
					const auto &ents = sys_behaviours.get_entities(s).get(prev_time);
					d.attached_ents.reserve(std::size(ents));
					for (const auto& e : ents)
					{
						if (e.second <= current_time)
							d.attached_ents.emplace_back(e.first);
					}
				}

				if (s.system->on_disconnect)
					d.removed_ents = sys_behaviours.get_removed_entities(s);

				data.emplace_back(std::move(d));
			}
		}

		//call on_connect for new entities
		for (auto& u : data)
		{
			if (!std::empty(u.added_ents) && u.system->on_connect)
			{
				auto& sys_data = sys_behaviours.get_system_data(u.system->id);
				auto game_data = std::invoke(make_game_struct, u.system->id, std::move(u.added_ents), &interface, &sys_behaviours, prev_time, dt, players, &sys_data);
				detail::set_data(&game_data);
				std::invoke(u.system->on_connect);
			}
		}

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
				auto game_data = std::invoke(make_game_struct, unique_id::zero, ent_list{}, &interface, &sys_behaviours, prev_time, dt, players, nullptr);
				detail::set_data(&game_data);
				std::invoke(player_input_fn, *players, std::move(input_q));
			}
		}

		//call on_disconnect for removed entities
		for (auto& u : data)
		{
			if (!std::empty(u.removed_ents) && u.system->on_disconnect)
			{
				auto& sys_data = sys_behaviours.get_system_data(u.system->id);
				auto game_data = std::invoke(make_game_struct, u.system->id, std::move(u.removed_ents), &interface, &sys_behaviours, prev_time, dt, players, &sys_data);
				detail::set_data(&game_data);
				std::invoke(u.system->on_disconnect);
			}
		}

		//call on_tick for systems
		for (const auto& u : data)
		{
			if (!std::empty(u.attached_ents) && u.system->tick)
			{
				auto& sys_data = sys_behaviours.get_system_data(u.system->id);
				auto game_data = std::invoke(make_game_struct, u.system->id, std::move(u.attached_ents), &interface, &sys_behaviours, prev_time, dt, players, &sys_data);
				detail::set_data(&game_data);
				std::invoke(u.system->tick);
			}
		}

		return current_time;
	}
}