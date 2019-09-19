#include "hades/level_interface.hpp"

#include "hades/parallel_jobs.hpp"
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
	time_point update_level(time_point before_prev, time_point prev_time, time_duration dt,
		Interface& interface, system_behaviours<SystemType> &sys, MakeGameStructFn make_game_struct)
	{
		using job_data_type = typename SystemType::job_data_t;
		static_assert(std::is_invocable_r_v<job_data_type, MakeGameStructFn, unique_id,
			std::vector<entity_id>, Interface*, system_behaviours<SystemType>*, time_point, time_duration, system_data_t*>,
			"make_game_struct must return the correct job_data_type");

		const auto current_time = prev_time + dt;
		
		{
			const auto new_systems = sys.get_new_systems();
			for (const auto s : new_systems)
			{
				if (!s->on_create)
					continue;

				//TODO: pass currently attached ents
				auto game_data = std::invoke(make_game_struct, s->id, std::vector<entity_id>{}, &interface, &sys, prev_time, dt, &sys.get_system_data(s->id));
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
		std::vector<update_data> data;
		
		{
			auto &systems = sys.get_systems();
			for (auto& s : systems)
			{
				update_data d{ s.system };
				if (s.system->on_connect)
					d.added_ents = sys.get_new_entities(s);

				if (s.system->tick)
				{
					//only update entities that have passed their wake up time
					const auto &ents = sys.get_entities(s).get(prev_time);
					d.attached_ents.reserve(std::size(ents));
					for (const auto& e : ents)
					{
						if (e.second <= current_time)
							d.attached_ents.emplace_back(e.first);
					}
				}

				if (s.system->on_disconnect)
					d.removed_ents = sys.get_removed_entities(s);

				data.emplace_back(std::move(d));
			}
		}

		//call on_connect for new entities
		for (auto& u : data)
		{
			if (!std::empty(u.added_ents) && u.system->on_connect)
			{
				auto& sys_data = sys.get_system_data(u.system->id);
				auto game_data = std::invoke(make_game_struct, u.system->id, std::move(u.added_ents), &interface, &sys, prev_time, dt, &sys_data);
				detail::set_data(&game_data);
				std::invoke(u.system->on_connect);
			}
		}

		//call on_disconnect for removed entities
		for (auto& u : data)
		{
			if (!std::empty(u.removed_ents) && u.system->on_disconnect)
			{
				auto& sys_data = sys.get_system_data(u.system->id);
				auto game_data = std::invoke(make_game_struct, u.system->id, std::move(u.removed_ents), &interface, &sys, prev_time, dt, &sys_data);
				detail::set_data(&game_data);
				std::invoke(u.system->on_disconnect);
			}
		}

		//call on_tick for systems
		for (const auto& u : data)
		{
			if (!std::empty(u.attached_ents) && u.system->tick)
			{
				auto& sys_data = sys.get_system_data(u.system->id);
				auto game_data = std::invoke(make_game_struct, u.system->id, std::move(u.attached_ents), &interface, &sys, prev_time, dt, &sys_data);
				detail::set_data(&game_data);
				std::invoke(u.system->tick);
			}
		}

		return current_time;
	}
}