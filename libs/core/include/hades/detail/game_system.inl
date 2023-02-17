#include "hades/game_system.hpp"

#include <type_traits>

#include "hades/data.hpp"
#include "hades/logging.hpp"

namespace hades
{
	namespace detail
	{
		template<typename SystemResource, typename System>
		static inline System& install_system(unique_id sys,
			std::deque<System>& systems, std::vector<const SystemResource*>& sys_r)
		{
			//never install a system more than once.
			assert(std::none_of(std::begin(systems), std::end(systems),
				[sys](const auto& system) {
					return system.system->id == sys;
				}
			));

			const auto new_system = hades::data::get<SystemResource>(sys);
			sys_r.emplace_back(new_system);
            return systems.emplace_back(System{ new_system, {}, {}, {}, {} });
		}

		template<typename SystemResource, typename System>
		static inline System& find_system(unique_id id, std::deque<System>& systems,
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
	inline name_list system_behaviours<SystemType>::get_new_entities(SystemType &sys)
	{
		// preserve the capacity of sys.new_ents
		// out will only allocate as much as it needs
		auto out = sys.new_ents;

		//add the new ents to attached ents
		sys.attached_entities.insert(end(sys.attached_entities),
			begin(sys.new_ents), end(sys.new_ents));

		sys.new_ents.clear();
		return out;
	}

	template<typename SystemType>
	inline name_list system_behaviours<SystemType>::get_created_entities(SystemType& sys)
	{
		return std::exchange(sys.created_ents, {});
	}

	template<typename SystemType>
	inline name_list& system_behaviours<SystemType>::get_entities(SystemType& sys) const
	{
		return sys.attached_entities;
	}

	template<typename SystemType>
	inline name_list system_behaviours<SystemType>::get_removed_entities(SystemType &sys)
	{
		const auto iter = std::partition(begin(sys.attached_entities),
			end(sys.attached_entities), [&s = sys.removed_ents](auto& o) {
			return end(s) == std::find(begin(s), end(s), o);
		});
		sys.removed_ents.clear();

		// get the removed group, so we can call disconnect on them
		auto out = name_list{ iter, end(sys.attached_entities) };
		sys.attached_entities.erase(iter, end(sys.attached_entities));
		
		return out;
	}

	namespace detail
	{
		inline bool assert_system_already_attached(object_ref o, name_list& s) noexcept
		{
			return std::find_if(s.begin(), s.end(), [o](auto&& ent) {
				return ent.object == o;
			}) != s.end();
		}
	}

	template<typename SystemType>
	inline void system_behaviours<SystemType>::attach_system(object_ref entity, unique_id sys)
	{
		//systems cannot be created or destroyed while we are editing the entity list
        auto& system = detail::find_system<typename SystemType::system_t>(sys, _systems, _new_systems);

		//not being already_attached implies a bug in the game api
		assert(!detail::assert_system_already_attached(entity, system.attached_entities));

		system.new_ents.emplace_back(typename name_list::value_type{ entity, time_point::min() });
		_dirty_systems = true;
		return;
	}

	template<typename SystemType>
	inline void system_behaviours<SystemType>::attach_system_from_load(object_ref entity, unique_id sys)
	{
		//systems cannot be created or destroyed while we are editing the entity list
        auto& system = detail::find_system<typename SystemType::system_t>(sys, _systems, _new_systems);
		auto& ent_list = system.attached_entities;

		//check that we arent double attaching
		{
			//TOD: this is possible due to errors in save files, i think, need to double check
			// and make into assert if its only a dev bug
			auto found = std::find_if(ent_list.begin(), ent_list.end(), [entity](auto&& ent) {
				return ent.object == entity;
				});

			if (found != ent_list.end())
			{
				const auto message = "The requested entityid is already attached to this system. EntityId: "
					+ to_string(entity) + ", System: " + to_string(sys);
				LOGERROR(message);
				throw system_error{ message };
			}
		}

		ent_list.emplace_back(typename name_list::value_type{ entity, time_point::min() });
		system.created_ents.emplace_back(typename name_list::value_type{ entity, time_point::min() });
		_dirty_systems = true;
		return;
	}

	namespace detail
	{
		template<typename SystemType>
		inline static void detach_system_impl(object_ref e, SystemType& sys)
		{			
			sys.removed_ents.emplace_back(typename name_list::value_type{ e, time_point::min() });
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
		//we add this obj to the removal list for all systems
		// we check if they were actually attached when
		// we grab the removal list
		for (auto& system : _systems)
			detail::detach_system_impl(e, system);

		_dirty_systems = true;
		return;
	}

	template<typename SystemType>
	inline void system_behaviours<SystemType>::sleep_entity(object_ref e, unique_id s, time_point b)
	{
		auto& sys = detail::find_system(s, _systems, _new_systems);
		// TODO: this might be a perf issue
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
		inline const std::decay_t<System>* make_system(unique_id id, CreateFunc on_create,
			ConnectFunc on_connect, DisconnectFunc on_disconnect,
			TickFunc on_tick, DestroyFunc on_destroy, data::data_manager &data)
		{
			using namespace std::string_literals;
			auto system_type = "game-system"s;
			if constexpr (std::is_same_v<System, resources::render_system>)
				system_type = "render-system"s;

			auto sys = data.find_or_create<std::decay_t<System>>(id, {}, system_type);

			if (!sys)
				throw system_error{ "unable to create requested system"s };

			sys->on_create = on_create;
			sys->on_connect = on_connect;
			sys->on_disconnect = on_disconnect;
			sys->tick = on_tick;
			sys->on_destroy = on_destroy;
			return sys;
		}
	}

	template<typename CreateFunc, typename ConnectFunc, typename DisconnectFunc, typename TickFunc, typename DestroyFunc>
	inline const resources::system* make_system(unique_id id, CreateFunc on_create, ConnectFunc on_connect, DisconnectFunc on_disconnect, TickFunc on_tick, DestroyFunc on_destroy, data::data_manager &data)
	{
		return detail::make_system<resources::system, system_job_data>(id, on_create, on_connect, on_disconnect, on_tick, on_destroy, data);
	}

	template<typename CreateFunc, typename ConnectFunc, typename DisconnectFunc, typename TickFunc, typename DestroyFunc>
	inline const resources::render_system* make_render_system(unique_id id,
		CreateFunc on_create, ConnectFunc on_connect, DisconnectFunc on_disconnect,
		TickFunc on_tick, DestroyFunc on_destroy, data::data_manager &d)
	{
		//TODO: pass const render_job_data instead
		return detail::make_system<resources::render_system, render_job_data>(id, on_create, on_connect, on_disconnect, on_tick, on_destroy, d);
	}
}
