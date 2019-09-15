#include "hades/level_interface.hpp"

#include "hades/parallel_jobs.hpp"
#include "hades/properties.hpp"
#include "hades/console_variables.hpp"

namespace hades
{
	template<typename SystemType>
	inline common_implementation<SystemType>::common_implementation(const level_save& sv) 
		: common_implementation_base{sv}
	{}

	template<typename SystemType>
	inline std::vector<SystemType> common_implementation<SystemType>::get_systems() const
	{
		return _systems;
	}

	template<typename SystemType>
	inline std::vector<const typename SystemType::system_t*> common_implementation<SystemType>::get_new_systems() const
	{
		return _new_systems;
	}

	template<typename SystemType>
	inline void common_implementation<SystemType>::clear_new_systems()
	{
		_new_systems.clear();
	}

	template<typename SystemType>
	inline system_data_t& common_implementation<SystemType>::get_system_data(unique_id key)
	{
		return _system_data[key];
	}

	//TODO: deprecate this
	namespace detail
	{
		template<typename SystemResource, typename System>
		static inline System& install_system_old(unique_id sys,
			std::vector<System>& systems, std::vector<const SystemResource*>& sys_r)
		{
			//never install a system more than once.
			assert(std::none_of(std::begin(systems), std::end(systems),
				[sys](const auto & system) {
					return system.system->id == sys;
				}
			));

			const auto new_system = hades::data::get<SystemResource>(sys);
			sys_r.emplace_back(new_system);
			return systems.emplace_back(new_system);
		}

		template<typename SystemResource, typename System>
		static inline System& find_system_old(unique_id id, std::vector<System>& systems, 
			std::vector<const SystemResource*> &sys_r)
		{
			for (auto& s : systems)
			{
				if (s.system->id == id)
					return s;
			}

			return install_system_old<SystemResource>(id, systems, sys_r);
		}
	}

	template<typename SystemType>
	inline void common_implementation<SystemType>::attach_system(entity_id entity, unique_id sys, time_point t)
	{
		//systems cannot be created or destroyed while we are editing the entity list
		auto& system = detail::find_system_old<SystemType::system_t>(sys, _systems, _new_systems);
		
		auto updated = system.attached_entities;

		if (updated.empty())
			updated.set(time_point{ nanoseconds{-1} }, {});

		auto ent_list = updated.get(t);
		auto found = std::find_if(ent_list.begin(), ent_list.end(), [entity](auto&& ent) {
			return ent.first == entity;
		});

		if (found != ent_list.end())
		{
			const auto message = "The requested entityid is already attached to this system. EntityId: "
				+ to_string(entity) + ", System: " + "err" + ", at time: " +
				to_string(std::chrono::duration_cast<seconds_float>(t.time_since_epoch()).count()) + "s";
			//ent is already attached
			throw system_already_attached{ message };
		}

		ent_list.emplace_back(entity, time_point{});
		updated.insert(t, std::move(ent_list));

		std::swap(updated, system.attached_entities);
		return;
	}

	template<typename SystemType>
	inline void common_implementation<SystemType>::detach_system(entity_id entity, unique_id sys, time_point t)
	{
		auto& system = detail::find_system_old<SystemType::system_t>(sys, _systems, _new_systems);
		auto ents = system.attached_entities;
		auto ent_list = ents.get(t);
		auto found = std::find_if(ent_list.begin(), ent_list.end(), [entity](auto&& ent) {
			return entity == ent.first;
		});

		if (found == ent_list.end())
		{
			const auto message = "The requested entityid isn't attached to this system. EntityId: "
				+ to_string(entity) + ", System: " + "err" + ", at time: " +
				to_string(std::chrono::duration_cast<seconds_float>(t.time_since_epoch()).count()) + "s";
			throw system_already_attached{ message };
		}

		ent_list.erase(found);
		ents.insert(t, ent_list);
		std::swap(system.attached_entities, ents);
		return;
		//TODO: call destroy system
	}

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

		namespace old
		{
			static inline resources::curve_types::collection_object_ref get_added_entites(const name_list& nl_curve, time_point last_frame, time_point this_frame)
			{
				assert(!std::empty(nl_curve));
				auto prev = nl_curve.get(last_frame);
				auto next = nl_curve.get(this_frame);

				std::sort(std::begin(prev), std::end(prev));
				std::sort(std::begin(next), std::end(next));

				static auto output = std::vector<attached_ent>{};
				output.clear();

				std::set_difference(std::begin(next), std::end(next),
					std::begin(prev), std::end(prev),
					std::back_inserter(output));

				auto ret = std::vector<entity_id>{};
				ret.reserve(std::size(output));

				for (const auto& o : output)
					ret.emplace_back(o.first);

				return ret;
			}

			static inline resources::curve_types::collection_object_ref get_removed_entites(const name_list& nl_curve, time_point last_frame, time_point this_frame)
			{
				assert(!std::empty(nl_curve));
				auto prev = nl_curve.get(last_frame);
				auto next = nl_curve.get(this_frame);

				std::sort(std::begin(prev), std::end(prev));
				std::sort(std::begin(next), std::end(next));

				static auto output = std::vector<attached_ent>{};
				output.clear();

				std::set_difference(std::begin(prev), std::end(prev),
					std::begin(next), std::end(next),
					std::back_inserter(output));

				auto ret = std::vector<entity_id>{};
				ret.reserve(std::size(output));

				for (const auto& o : output)
					ret.emplace_back(o.first);

				return ret;
			}
		}

		template<typename ImplementationType, typename MakeGameStructFn>
		time_point update_level_sync(time_point before_prev, time_point prev_time, time_duration dt,
			ImplementationType& impl, MakeGameStructFn make_game_struct)
		{
			const auto current_time = prev_time + dt;

			using system_type = typename ImplementationType::system_type;
			using job_data_type = typename system_type::job_data_t;

			const auto new_systems = impl.get_new_systems();
			const auto systems = impl.get_systems();

			struct update_data
			{
				const ImplementationType::system_resource *system = nullptr;
				resources::curve_types::collection_object_ref added_ents, attached_ents, removed_ents;
			};

			//collect entity lists upfront, so that they don't change mid update
			std::vector<update_data> data;
			for (const auto &s : systems)
			{
				update_data d{ s.system };
				if (s.system->on_connect)
					d.added_ents = old::get_added_entites(s.attached_entities, before_prev, prev_time);

				if (s.system->tick)
				{
					const auto &ents = s.attached_entities.get(prev_time);
					d.attached_ents.reserve(std::size(ents));
					for (const auto& e : ents)
						d.attached_ents.emplace_back(e.first);
				}

				if(s.system->on_disconnect)
					d.removed_ents = old::get_removed_entites(s.attached_entities, before_prev, prev_time);

				data.emplace_back(std::move(d));
			}

			for (const auto s : new_systems)
			{
				if (!s->on_create)
					continue;

				auto game_data = std::invoke(make_game_struct, bad_entity, &impl, prev_time, dt, &impl.get_system_data(s->id));
				set_data(&game_data);
				std::invoke(s->on_create);
			}

			impl.clear_new_systems();
			//call on_connect for new entities
			for (const auto &u : data)
			{
				auto& sys_data = impl.get_system_data(u.system->id);
				auto game_data = std::invoke(make_game_struct, bad_entity, &impl, prev_time, dt, &sys_data);
				for (const auto e : u.added_ents)
				{
					game_data.entity = e;
					set_data(&game_data);
					std::invoke(u.system->on_connect);
				}
			}

			//call on_disconnect for removed entities
			for (const auto &u : data)
			{
				auto& sys_data = impl.get_system_data(u.system->id);
				auto game_data = std::invoke(make_game_struct, bad_entity, &impl, prev_time, dt, &sys_data);
				for (const auto e : u.removed_ents)
				{
					game_data.entity = e;
					set_data(&game_data);
					std::invoke(u.system->on_disconnect);
				}
			}

			//call on_tick for systems
			for (const auto &u : data)
			{
				auto& sys_data = impl.get_system_data(u.system->id);
				auto game_data = std::invoke(make_game_struct, bad_entity, &impl, prev_time, dt, &sys_data);

				for (const auto e : u.attached_ents)
				{
					game_data.entity = e;
					set_data(&game_data);
					std::invoke(u.system->tick);
				}
			}

			return current_time;
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