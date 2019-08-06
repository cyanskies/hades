#include "hades/game_system.hpp"

#include <type_traits>

#include "hades/data.hpp"
#include "hades/transaction.hpp"

namespace hades
{
	namespace detail
	{
		template<typename JobData, typename Func>
		inline auto make_system_func(Func f)
		{
			//this function is a bit ugly
			//bool func(job_system&, JobData)
			if constexpr (std::is_invocable_r_v<bool, Func, job_system&, JobData>)
			{
				return [f](job_system &j, JobData job_data)->bool {
					return std::invoke(f, j, std::move(job_data));
				};
			}
			//non-bool func(job_system&, JobData)
			else if constexpr (std::is_invocable_v<Func, job_system&, JobData>)
			{
				return [f](job_system &j, JobData job_data)->bool {
					std::invoke(f, j, std::move(job_data));
					return true;
				};
			}
			//bool func(JobData)
			else if constexpr (std::is_invocable_r_v<bool, Func, JobData>)
			{
				return [f](job_system &j, JobData job_data)->bool {
					return std::invoke(f, std::move(job_data));
				};
			}
			//non-bool func(JobData)
			else if constexpr (std::is_invocable_v<Func, JobData>)
			{
				return [f](job_system &j, JobData job_data)->bool {
					std::invoke(f, std::move(job_data));
					return true;
				};
			}
			//bool func(job_system&)
			else if constexpr (std::is_invocable_r_v<bool, Func, job_system&>)
			{
				return [f](job_system& j, JobData job_data)->bool {
					return std::invoke(f, j);
				};
			}
			//non-bool func(job_system&)
			else if constexpr (std::is_invocable_v<Func, job_system&>)
			{
				return [f](job_system& j, JobData job_data)->bool {
					std::invoke(f, j);
					return true;
				};
			}
			//bool func()
			else if constexpr (std::is_invocable_r_v<bool, Func>)
			{
				return [f](job_system& j, JobData job_data)->bool {
					return std::invoke(f);
				};
			}
			//non-bool func()
			else if constexpr (std::is_invocable_v<Func>)
			{
				return [f](job_system& j, JobData job_data)->bool {
					std::invoke(f);
					return true;
				};
			}
			//not provided
			else if constexpr (std::is_null_pointer_v<Func>)
				return f;
			else //not invocable
				static_assert(always_false<Func>::value,
					"system functions must be a function object with the following definition: bool func(hades::job_system&, hades::system_job_data), the return and job_system ref are optional");
		}

		template<typename System, typename JobData, typename CreateFunc, typename ConnectFunc, typename DisconnectFunc, typename TickFunc, typename DestroyFunc>
		inline void make_system(unique_id id, CreateFunc on_create, ConnectFunc on_connect, DisconnectFunc on_disconnect, TickFunc on_tick, DestroyFunc on_destroy, data::data_manager &data)
		{
			auto sys = data.find_or_create<System>(id, unique_id::zero);

			if (!sys)
				throw system_error("unable to create requested system");

			sys->on_create = make_system_func<JobData>(on_create);
			sys->on_connect = make_system_func<JobData>(on_connect);
			sys->on_disconnect = make_system_func<JobData>(on_disconnect);
			sys->tick = make_system_func<JobData>(on_tick);
			sys->on_destroy = make_system_func<JobData>(on_destroy);
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
		transaction& get_game_transaction();
		bool get_game_data_async();

		template<typename T>
		curve<T> get_game_curve(game_interface *l, curve_index_t i)
		{
			assert(l);
			auto& curves = l->get_curves();
			auto& curve_map = hades::get_curve_list<T>(curves);

			if (detail::get_game_data_async())
				return detail::get_game_transaction().get(i, curve_map);
			else
				return curve_map.get_no_async(i);
		}

		template<typename T>
		game::curve_keyframe<T> get_game_curve_frame(game_interface* l, curve_index_t i, time_point t)
		{
			assert(l);
			auto& curves = l->get_curves();
			auto& curve_map = hades::get_curve_list<T>(curves);

			const auto &curve = [i, t, &curve_map]() {
				if (detail::get_game_data_async())
					return detail::get_game_transaction().get(i, curve_map);
				else
					return curve_map.get_no_async(i);
			}();

			const auto [time, value] = curve.getPrevious(t);
			return { value, time };
		}

		template<typename T>
		void set_game_curve(game_interface *l, curve_index_t i, curve<T> c)
		{
			assert(l);

			auto& curves = l->get_curves();
			auto& target_curve_list = hades::get_curve_list<T>(curves);

			if (detail::get_game_data_async())
				detail::get_game_transaction().set(target_curve_list, i, std::move(c));
			else
				target_curve_list.set(i, std::move(c));

			return;
		}

		template<typename T>
		void set_game_value(game_interface *l, curve_index_t i, time_point t, T&& v)
		{
			assert(l);

			auto& curves = l->get_curves();
			auto& target_curve_type = hades::get_curve_list<T>(curves);

			if (detail::get_game_data_async())
			{
				auto c = detail::get_game_transaction().peek(i, target_curve_type);
				c.set(t, std::forward<T>(v));
				detail::get_game_transaction().set(target_curve_type, i, std::move(c));
			}
			else
			{
				auto &c = target_curve_type.get_no_async(i);
				c.set(t, std::forward<T>(v));
				//target_curve_type.set(i, std::move(c));
			}

			return;
		}

		render_job_data* get_render_data_ptr();
		transaction &get_render_transaction();
		bool get_render_data_async();

		template<typename T>
		curve<T> get_render_curve(game_interface *l, curve_index_t i)
		{
			assert(l);

			auto& curves = l->get_curves();
			auto& curve_map = hades::get_curve_list<T>(curves);

			if (detail::get_render_data_async())
				return detail::get_render_transaction().get(i, curve_map);
			else
				return curve_map.get_no_async(i);
		}
	}

	namespace game
	{
		template<typename T>
		void create_system_value(unique_id key, T&& value)
		{
			auto ptr = detail::get_game_data_ptr();
			assert(ptr);
			ptr->system_data->create(key, std::forward<T>(value));
		}

		template<typename T>
		void set_system_value(unique_id key, T&& value)
		{
			auto ptr = detail::get_game_data_ptr();
			assert(ptr);
			if (detail::get_game_data_async())
				detail::get_game_transaction().set(*ptr->system_data, key, std::forward<T>(value));
			else
				ptr->system_data->set(key, std::forward<T>(value));
		}

		template<typename T>
		T get_system_value(unique_id key)
		{
			auto ptr = detail::get_game_data_ptr();
			assert(ptr);
			if (detail::get_game_data_async())
				return detail::get_game_transaction().get<T>(key, *ptr->system_data);
			else
				return ptr->system_data->get_no_async<T>(key);
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
			const auto curve = get_curve<T>(i);
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
		void create_system_value(unique_id key, T&& value)
		{
			auto ptr = detail::get_render_data_ptr();
			assert(ptr);
			ptr->system_data->create(key, std::forward<T>(value));
			return;
		}

		template<typename T>
		T get_system_value(unique_id key)
		{
			auto ptr = detail::get_render_data_ptr();
			assert(ptr);

			if (detail::get_render_data_async())
				return detail::get_render_transaction().get<T>(key, *ptr->system_data);
			else
				return ptr->system_data->get_no_async<T>(key);
		}

		template<typename T>
		void set_system_value(unique_id key, T&& value)
		{
			auto ptr = detail::get_render_data_ptr();
			assert(ptr);

			if (detail::get_render_data_async())
				return detail::get_render_transaction().set(*ptr->system_data, key, std::forward<T>(value));
			else
				return ptr->system_data->set(key, std::forward<T>(value));
		}
	}

	namespace render::level
	{
		template<typename T>
		curve<T> get_curve(variable_id v)
		{
			auto ent = get_object();
			return get_curve<T>({ ent, v });
		}

		template<typename T>
		curve<T> get_curve(curve_index_t i)
		{
			auto ptr = detail::get_render_data_ptr();
			assert(ptr);

			return detail::get_render_curve<T>(ptr->level_data, i);
		}

		template<typename T>
		T get_value(curve_index_t i, time_point t)
		{
			auto curve = get_curve<T>(i);
			return curve.get(t);
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