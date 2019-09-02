#include "hades/game_system.hpp"

#include <type_traits>

#include "hades/data.hpp"
#include "hades/transaction.hpp"

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
	inline void system_behaviours<SystemType>::attach_system(entity_id entity, unique_id sys, time_point t)
	{
		//systems cannot be created or destroyed while we are editing the entity list
		auto& system = detail::find_system<SystemType::system_t>(sys, _systems, _new_systems);

		auto updated = system.attached_entities;

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
		updated.insert(t, std::move(ent_list));

		std::swap(updated, system.attached_entities);
		return;
	}

	template<typename SystemType>
	inline void system_behaviours<SystemType>::detach_system(entity_id entity, unique_id sys, time_point t)
	{
		auto& system = detail::find_system<SystemType::system_t>(sys, _systems, _new_systems);
		auto ents = system.attached_entities;
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
		std::swap(system.attached_entities, ents);
		return;
		//TODO: call destroy system
		//we dont have the data we need for this
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

	namespace detail
	{
		system_job_data* get_game_data_ptr();
		game_interface* get_game_level_ptr();

		template<typename T>
		curve<T> &get_game_curve_ref(game_interface* l, curve_index_t i)
		{
			assert(l);
			auto& curves = l->get_curves();
			auto& curve_map = hades::get_curve_list<T>(curves);

			return curve_map.find(i)->second;
		}

		template<typename T>
		game::curve_keyframe<T> get_game_curve_frame(game_interface* l, curve_index_t i, time_point t)
		{
			assert(l);
			auto& curves = l->get_curves();
			auto& curve_map = hades::get_curve_list<T>(curves);

			const auto& curve = curve_map.find(i)->second;

			const auto [time, value] = curve.getPrevious(t);
			return { value, time };
		}

		template<typename T>
		void set_game_curve(game_interface *l, curve_index_t i, curve<T> c)
		{
			assert(l);

			auto& curves = l->get_curves();
			auto& target_curve_list = hades::get_curve_list<T>(curves);

			target_curve_list.insert_or_assign(i, std::move(c));

			return;
		}

		template<typename T>
		void set_game_value(game_interface *l, curve_index_t i, time_point t, T&& v)
		{
			assert(l);

			auto& curves = l->get_curves();
			auto& target_curve_type = hades::get_curve_list<T>(curves);
			auto &c = target_curve_type.find(i)->second;
			c.set(t, std::forward<T>(v));
		
			return;
		}

		render_job_data* get_render_data_ptr();

		template<typename T>
		const curve<T>& get_render_curve(const common_interface *l, curve_index_t i)
		{
			assert(l);

			auto& curves = l->get_curves();
			auto& curve_map = hades::get_curve_list<T>(curves);
			return curve_map.find(i)->second;
		}
	}

	namespace game
	{
		template<typename T>
		void set_system_data(T&& value)
		{
			auto ptr = detail::get_game_data_ptr();
			assert(ptr);
			*ptr->system_data = std::forward<T>(value);
		}

		template<typename T>
		T &get_system_data()
		{
			auto ptr = detail::get_game_data_ptr();
			assert(ptr);
			return *std::any_cast<T>(ptr->system_data);
		}
	}

	namespace game::level
	{
		template<typename T>
		curve<T> get_curve(object_ref e, variable_id v)
		{
			return get_curve<T>(curve_index_t{ e, v });
		}

		template<typename T>
		curve<T> get_curve(variable_id v)
		{
			auto ent = game::get_object();
			return get_curve<T>({ ent, v });
		}

		template<typename T>
		curve<T> get_curve(curve_index_t i)
		{
			auto ptr = detail::get_game_data_ptr();
			assert(ptr);

			return detail::get_game_curve<T>(ptr->level_data, i);
		}

		template<typename T>
		curve_keyframe<T> get_keyframe(curve_index_t index, time_point t)
		{
			auto ptr = detail::get_game_data_ptr();
			assert(ptr);

			return detail::get_game_curve_frame<T>(ptr->level_data, index, t);
		}

		template<typename T>
		curve_keyframe<T> get_keyframe(object_ref o, variable_id i, time_point t)
		{
			return get_keyframe<T>(curve_index_t{ o, i }, t);
		}

		template<typename T>
		curve_keyframe<T> get_keyframe(variable_id i, time_point t)
		{
			return get_keyframe<T>(get_object(), i, t);
		}

		template<typename T>
		curve_keyframe<T> get_keyframe(curve_index_t index)
		{
			return get_keyframe<T>(index, get_last_time());
		}

		template<typename T>
		curve_keyframe<T> get_keyframe(object_ref o, variable_id i)
		{
			return get_keyframe<T>(o, i, get_last_time());
		}

		template<typename T>
		curve_keyframe<T> get_keyframe(variable_id i)
		{
			return get_keyframe<T>(get_object(), i, get_last_time());
		}

		template<typename T>
		inline T get_value(object_ref e, variable_id v, time_point t)
		{
			return get_value<T>({ e,v }, t);
		}

		template<typename T>
		T get_value(curve_index_t i, time_point t)
		{
			const auto& curve = detail::get_game_curve_ref<T>(detail::get_game_data_ptr()->level_data, i);
			return curve.get(t);
		}

		template<typename T>
		T get_value(variable_id v, time_point t)
		{
			return get_value<T>(curve_index_t{ game::get_object(), v }, t);
		}

		template<typename T>
		T get_value(object_ref o, variable_id v)
		{
			return get_value<T>(curve_index_t{ o, v });
		}

		template<typename T>
		T get_value(curve_index_t i)
		{
			return get_value<T>(i, get_last_time());
		}

		template<typename T>
		inline T get_value(variable_id v)
		{
			return get_value<T>(curve_index_t{ get_object(), v });
		}

		template<typename T>
		inline void set_curve(object_ref e, variable_id v, curve<T> c)
		{
			return set_curve<T>(curve_index_t{ e, v }, std::move(c));
		}

		template<typename T>
		void set_curve(curve_index_t i, curve<T> c)
		{
			auto ptr = detail::get_game_data_ptr();
			assert(ptr);

			return detail::set_game_curve(ptr->level_data, i, std::move(c));
		}

		template<typename T>
		inline void set_curve(variable_id v, curve<T> c)
		{
			const auto e = get_object();
			return set_curve(curve_index_t{ e, v }, std::move(c));
		}

		template<typename T>
		void set_value(object_ref o, variable_id v, time_point t, T&& val)
		{
			return set_value(curve_index_t{ o, v }, t, std::forward<T>(val));
		}

		template<typename T>
		void set_value(curve_index_t i, time_point t, T&& v)
		{
			auto ptr = detail::get_game_data_ptr();
			assert(ptr);

			return detail::set_game_value(ptr->level_data, i, t, std::forward<T>(v));
		}

		template<typename T>
		void set_value(variable_id v, time_point t, T&& val)
		{
			return set_value(curve_index_t{ get_object(), v }, t, std::forward<T>(val));
		}

		template<typename T>
		void set_value(object_ref e, variable_id v, T&& val)
		{
			return set_value(curve_index_t{ e, v }, get_time(), std::forward<T>(val));
		}

		template<typename T>
		void set_value(curve_index_t i, T&& t)
		{
			return set_value(i, get_time(), std::forward<T>(t));
		}

		template<typename T>
		void set_value(variable_id v, T&& val)
		{
			return set_value(curve_index_t{ get_object(), v }, std::forward<T>(val));
		}
	}

	namespace render
	{
		template<typename T>
		T &get_system_data()
		{
			auto ptr = detail::get_render_data_ptr();
			assert(ptr);
			auto ret = std::any_cast<T>(ptr->system_data);
			assert(ret);
			return *ret;
		}

		template<typename T>
		void set_system_data(T value)
		{
			auto ptr = detail::get_render_data_ptr();
			assert(ptr);
			ptr->system_data->emplace<std::decay_t<T>>(std::move(value));
		}
	}

	namespace render::level
	{
		template<typename T>
		const curve<T>& get_curve(variable_id v)
		{
			auto ent = get_object();
			return get_curve<T>({ ent, v });
		}

		template<typename T>
		const curve<T>& get_curve(curve_index_t i)
		{
			auto ptr = detail::get_render_data_ptr();
			assert(ptr);

			return detail::get_render_curve<T>(ptr->level_data, i);
		}

		template<typename T>
		const T &get_value(curve_index_t i, time_point t)
		{
			auto &curve = get_curve<T>(i);
			return curve.get_ref(t);
		}

		/*template<typename T>
		void set_curve(curve_index_t i, T&& value)
		{
			auto ptr = detail::get_render_data_ptr();
			assert(ptr);

			auto& curves = ptr->level_data->get_curves();
			auto& curve_map = get_curve_list<std::decay_t<T>>(curves);

			if (detail::get_render_data_async())
				return detail::get_render_transaction().set(curve_map, i, std::forward(value));
			else
				return curve_map.set(i, std::forward(value));
		}*/
	}
}