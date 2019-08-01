#include "hades/level_interface.hpp"

#include "hades/parallel_jobs.hpp"

namespace hades
{
	template<typename SystemType>
	inline common_implementation<SystemType>::common_implementation(const level_save& sv) 
		: common_implementation_base{sv}
	{}

	template<typename SystemType>
	inline std::vector<SystemType> common_implementation<SystemType>::get_systems() const
	{
		const auto lock = std::scoped_lock{ _system_list_mut };
		return _systems;
	}

	template<typename SystemType>
	inline std::vector<const typename SystemType::system_t*> common_implementation<SystemType>::get_new_systems() const
	{
		const auto lock = std::scoped_lock{ _system_list_mut };
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

	namespace detail
	{
		template<typename SystemResource, typename System>
		static inline System& install_system(unique_id sys,
			std::vector<System>& systems, std::vector<const SystemResource*>& sys_r, std::mutex& mutex)
		{
			//we MUST already be locked before we get here
			assert(!mutex.try_lock());

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
		static inline System& find_system(unique_id id, std::vector<System>& systems, 
			std::vector<const SystemResource*> &sys_r, std::mutex& mutex)
		{
			//we MUST already be locked before we get here
			assert(!mutex.try_lock());

			for (auto& s : systems)
			{
				if (s.system->id == id)
					return s;
			}

			return install_system<SystemResource>(id, systems, sys_r, mutex);
		}
	}

	template<typename SystemType>
	inline void common_implementation<SystemType>::attach_system(entity_id entity, unique_id sys, time_point t)
	{
		const auto lock = std::lock_guard{ _system_list_mut };
		//systems cannot be created or destroyed while we are editing the entity list
		auto& system = detail::find_system<SystemType::system_t>(sys, _systems, _new_systems, _system_list_mut);
		
		thread_local static name_list old, updated;
		do {
			updated = old = system.attached_entities.get();

			if (updated.empty())
				updated.set(time_point{ nanoseconds{-1} }, {});

			auto ent_list = updated.get(t);
			auto found = std::find(ent_list.begin(), ent_list.end(), entity);
			if (found != ent_list.end())
			{
				const auto message = "The requested entityid is already attached to this system. EntityId: "
					+ to_string(entity) + ", System: " + "err" + ", at time: " +
					to_string(std::chrono::duration_cast<seconds_float>(t.time_since_epoch()).count()) + "s";
				//ent is already attached
				throw system_already_attached{ message };
			}

			ent_list.emplace_back(entity);
			updated.insert(t, ent_list);
		} while (!system.attached_entities.compare_exchange(old, std::move(updated)));
	}

	template<typename SystemType>
	inline void common_implementation<SystemType>::detach_system(entity_id entity, unique_id sys, time_point t)
	{
		const auto lock = std::lock_guard{ _system_list_mut };

		auto& system = detail::find_system<SystemType::system_t>(sys, _systems, _new_systems, _system_list_mut);
		auto ents = system.attached_entities.get();
		auto ent_list = ents.get(t);
		auto found = std::find(ent_list.begin(), ent_list.end(), entity);
		if (found == ent_list.end())
		{
			const auto message = "The requested entityid isn't attached to this system. EntityId: "
				+ to_string(entity) + ", System: " + "err" + ", at time: " +
				to_string(std::chrono::duration_cast<seconds_float>(t.time_since_epoch()).count()) + "s";
			throw system_already_attached{ message };
		}

		ent_list.erase(found);
		ents.insert(t, ent_list);
		system.attached_entities = ents;

		//TODO: call destroy system?
		// maybe system on-destroy doesn't need to exist
	}

	template<typename GameStruct, typename Func>
	static constexpr auto make_job_function_wrapper(Func f) noexcept
	{
		if constexpr (std::is_same_v<GameStruct, system_job_data>)
		{
			return [f](job_system& j, system_job_data d)->bool {
				set_game_data(&d);

				const auto ret = std::invoke(f, j, d);
				if (ret)
					return finish_game_job();
				else
					abort_game_job();

				return ret;
			};
		}
		else
		{
			return [f](job_system& j, render_job_data d)->bool {
				set_render_data(&d);

				const auto ret = std::invoke(f, j, d);
				if (ret)
					return finish_render_job();
				else
					abort_render_job();

				return ret;
			};
		}
	}

	//generic update function for use in both client and server game instances
	template<typename ImplementationType, typename MakeGameStructFn>
	time_point update_level(job_system &jobsys, time_point before_prev, time_point prev_time, time_duration dt,
		ImplementationType &impl, MakeGameStructFn make_game_struct)
	{
		const auto current_time = prev_time + dt;

		using system_type = typename ImplementationType::system_type;
		using job_data_type = typename system_type::job_data_t;

		static_assert(std::is_invocable_r_v<job_data_type, MakeGameStructFn,
			entity_id, game_interface*, time_point, time_duration, system_data_t*>,
			"make_game_struct must return the correct job_data_type");

		assert(jobsys.ready());
		const auto on_create_parent = jobsys.create();
		const auto new_systems = impl.get_new_systems();

		std::vector<job*> jobs;
		for (const auto s : new_systems)
		{
			if (!s->on_create)
				continue;

			const auto j = jobsys.create_child(on_create_parent, make_job_function_wrapper<job_data_type>(s->on_create),
				std::invoke(make_game_struct, bad_entity, &impl, prev_time, dt, &impl.get_system_data(s->id)));

			jobs.emplace_back(j);
		}

		impl.clear_new_systems();
		const auto systems = impl.get_systems();

		//call on_connect for new entities
		const auto on_connect_parent = jobsys.create();
		for (const auto s : systems)
		{
			if (!s.system->on_connect)
				continue;

			const auto ents = get_added_entites(s.attached_entities, before_prev, prev_time);
			auto& sys_data = impl.get_system_data(s.system->id);

			for (const auto e : ents)
			{
				const auto j = jobsys.create_child_rchild(on_connect_parent,
					on_create_parent, make_job_function_wrapper<job_data_type>(s.system->on_connect),
					std::invoke(make_game_struct, e, &impl, prev_time, dt, &sys_data ));

				jobs.emplace_back(j);
			}
		}

		//call on_disconnect for removed entities
		const auto on_disconnect_parent = jobsys.create();
		for (const auto s : systems)
		{
			if (!s.system->on_disconnect)
				continue;

			const auto ents = get_removed_entites(s.attached_entities, before_prev, prev_time);
			auto& sys_data = impl.get_system_data(s.system->id);

			for (const auto e : ents)
			{
				const auto j = jobsys.create_child_rchild(on_disconnect_parent,
					on_connect_parent, make_job_function_wrapper<job_data_type>(s.system->on_disconnect),
					std::invoke(make_game_struct, e, &impl, prev_time, dt, &sys_data));

				jobs.emplace_back(j);
			}
		}

		//call on_tick for systems
		const auto on_tick_parent = jobsys.create();
		for (auto& s : systems)
		{
			if (!s.system->tick)
				continue;

			const auto entities_curve = s.attached_entities.get();
			const auto ents = entities_curve.get(prev_time);

			auto& sys_data = impl.get_system_data(s.system->id);

			for (const auto e : ents)
			{
				const auto j = jobsys.create_child_rchild(on_tick_parent,
					on_disconnect_parent, make_job_function_wrapper<job_data_type>(s.system->tick),
					std::invoke(make_game_struct, e, &impl, prev_time, dt, &sys_data));

				jobs.emplace_back(j);
			}
		}

		jobsys.run(std::begin(jobs), std::end(jobs));
		jobsys.wait(on_tick_parent);
		jobsys.clear();

		return current_time;
	}

}