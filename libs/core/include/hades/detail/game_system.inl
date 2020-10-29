#include "hades/game_system.hpp"

#include <type_traits>

#include "hades/data.hpp"

namespace hades
{
	namespace detail
	{
		template<typename SystemResource, typename System>
		static inline System& install_system(unique_id sys,
			std::vector<System>& systems, std::vector<const SystemResource*>& sys_r)
		{
			//never install a system more than once.
			assert(std::none_of(std::begin(systems), std::end(systems),
				[sys](const auto& system) {
					return system.system->id == sys;
				}
			));

			const auto new_system = hades::data::get<SystemResource>(sys);
			sys_r.emplace_back(new_system);
			return systems.emplace_back(new_system);
		}

		template<typename SystemResource, typename System>
		static inline System& find_system(unique_id id, std::vector<System>& systems,
			std::vector<const SystemResource*>& sys_r)
		{
			for (auto& s : systems)
			{
				if (s.system->id == id)
					return s;
			}

			return install_system<SystemResource>(id, systems, sys_r);
		}
	}

	template<typename SystemType>
	inline void system_behaviours<SystemType>::set_attached(unique_id i, name_list c)
	{
		auto& s = detail::find_system(i, _systems, _new_systems);
		s.attached_entities = std::move(c);
		return;
	}

	template<typename SystemType>
	inline void system_behaviours<SystemType>::set_current_time(time_point t)
	{
		for (auto& s : _systems)
		{
			//added ents
			auto this_frame = s.attached_ents.getPrevious(t);
			auto prev_frame = s.attached_ents.getPrevious(this_frame.first - nanoseconds{ 1 });
			auto &prev = prev_frame.second;
			auto &next = this_frame.second;

			std::sort(std::begin(prev), std::end(prev));
			std::sort(std::begin(next), std::end(next));

			static auto output = resources::curve_types::collection_object_ref{};
			output.clear();

			std::set_difference(std::begin(next), std::end(next),
				std::begin(prev), std::end(prev),
				std::back_inserter(output));

			s.new_ents = std::move(output);

			output.clear();

			std::set_difference(std::begin(prev), std::end(prev),
				std::begin(next), std::end(next),
				std::back_inserter(output));

			s.removed_ents = std::move(output);
		}

		return;
	}

	template<typename SystemType>
	inline std::vector<object_ref> system_behaviours<SystemType>::get_new_entities(SystemType &sys)
	{
		auto new_ents = std::vector<object_ref>{};
		std::swap(sys.new_ents, new_ents);
		return new_ents;
	}

	template<typename SystemType>
	inline const name_list& system_behaviours<SystemType>::get_entities(SystemType& sys) const
	{
		return sys.attached_entities;
	}

	template<typename SystemType>
	inline std::vector<object_ref> system_behaviours<SystemType>::get_removed_entities(SystemType &sys)
	{
		auto ret = std::vector<object_ref>{};
		std::swap(sys.removed_ents, ret);
		return ret;
	}

	template<typename SystemType>
	inline void system_behaviours<SystemType>::attach_system(object_ref entity, unique_id sys)
	{
		//systems cannot be created or destroyed while we are editing the entity list
		auto& system = detail::find_system<SystemType::system_t>(sys, _systems, _new_systems);

		auto& ent_list = system.attached_entities;
		auto found = std::find_if(ent_list.begin(), ent_list.end(), [entity](auto&& ent) {
			return ent.first == entity;
		});

		if (found != ent_list.end())
		{
			const auto message = "The requested entityid is already attached to this system. EntityId: "
				+ to_string(entity) + ", System: " + to_string(sys);
			throw system_error{ message };// system_already_attached{ message };
		}

		ent_list.emplace_back(entity, time_point{});
		system.new_ents.emplace_back(entity);
		return;
	}

	namespace detail
	{
		template<typename SystemType>
		inline static void detach_system_impl(object_ref e, SystemType& sys)
		{
			
			//removed from the active list
			const auto remove_iter = std::find_if(std::begin(sys.attached_entities), std::end(sys.attached_entities), [e](const attached_ent& ent) {
				return e == ent.first;
			});

			if(remove_iter == end(sys.attached_entities))
				return;
			
			sys.removed_ents.emplace_back(e);
			*remove_iter = *rbegin(sys.attached_entities);
			sys.attached_entities.pop_back();

			return;
		}
	}

	template<typename SystemType>
	inline void system_behaviours<SystemType>::detach_system(object_ref entity, unique_id sys)
	{
		//TODO: deprecate
		auto& system = detail::find_system<SystemType::system_t>(sys, _systems, _new_systems);
		//detail::detach_system_impl(entity, system);
		return;
	}

	template<typename SystemType>
	inline void system_behaviours<SystemType>::detach_all(object_ref e)
	{
		for (auto& system : _systems)
			detail::detach_system_impl(e, system);
		return;
	}

	template<typename SystemType>
	inline void system_behaviours<SystemType>::sleep_entity(object_ref e, unique_id s, time_point, time_point b)
	{
		auto& sys = detail::find_system(s, _systems, _new_systems);
		for (auto& [entity, time] : sys.attached_entities)
		{
			if (e == entity)
			{
				time = b;
				return;
			}
		}

		throw system_error{ "Tried to sleep an entity from a system they weren't attached too" };
		return;
	}

	namespace detail
	{
		template<typename System, typename JobData, typename CreateFunc,
			typename ConnectFunc, typename DisconnectFunc, typename TickFunc,
			typename DestroyFunc>
		inline void make_system(unique_id id, CreateFunc on_create,
			ConnectFunc on_connect, DisconnectFunc on_disconnect,
			TickFunc on_tick, DestroyFunc on_destroy, data::data_manager &data)
		{
			auto sys = data.find_or_create<System>(id, unique_id::zero);

			if (!sys)
				throw system_error("unable to create requested system");

			sys->on_create = on_create;
			sys->on_connect = on_connect;
			sys->on_disconnect = on_disconnect;
			sys->tick = on_tick;
			sys->on_destroy = on_destroy;
			return;
		}
	}

	template<typename CreateFunc, typename ConnectFunc, typename DisconnectFunc, typename TickFunc, typename DestroyFunc>
	inline void make_system(unique_id id, CreateFunc on_create, ConnectFunc on_connect, DisconnectFunc on_disconnect, TickFunc on_tick, DestroyFunc on_destroy, data::data_manager &data)
	{
		detail::make_system<resources::system, system_job_data>(id, on_create, on_connect, on_disconnect, on_tick, on_destroy, data);
	}

	template<typename CreateFunc, typename ConnectFunc, typename DisconnectFunc, typename TickFunc, typename DestroyFunc>
	inline void make_render_system(unique_id id, CreateFunc on_create, ConnectFunc on_connect, DisconnectFunc on_disconnect, TickFunc on_tick, DestroyFunc on_destroy, data::data_manager &d)
	{
		//TODO: pass const render_job_data instead
		detail::make_system<resources::render_system, render_job_data>(id, on_create, on_connect, on_disconnect, on_tick, on_destroy, d);
	}
}
