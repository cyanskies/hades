#include "hades/level_interface.hpp"

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

		auto& system = detail::find_system<SystemType::system_t>(sys, _systems, _new_systems, _system_list_mut);
		name_list ents = system.attached_entities.get();

		if (ents.empty())
			ents.set(time_point{ nanoseconds{-1} }, {});

		auto ent_list = ents.get(t);
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
		ents.insert(t, ent_list);
		system.attached_entities = ents;
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
	}
}