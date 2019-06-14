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
		detail::make_system<resources::render_system, render_job_data>(id, on_create, on_connect, on_disconnect, on_tick, on_destroy, d);
	}

	namespace detail
	{
		inline render_job_data* get_render_data_ptr();
		inline transaction &get_render_transaction();
		inline bool get_render_data_async();
	}

	namespace render
	{
		template<typename T>
		void create_system_value(unique_id key, T&& value)
		{
			auto ptr = detail::get_render_data_ptr();
			assert(ptr);
			ptr->system_data.create(key, std::forward<T>(value));
			return;
		}

		template<typename T>
		T get_system_value(unique_id key)
		{
			auto ptr = detail::get_render_data_ptr();
			assert(ptr);

			if (detail::get_render_data_async())
				return detail::get_render_transaction().get<T>(key, ptr->system_data);
			else
				return ptr->system_data.get_no_async<T>(key);
		}

		template<typename T>
		void set_system_value(unique_id key, T&& value)
		{
			auto ptr = detail::get_render_data_ptr();
			assert(ptr);

			if (detail::get_render_data_async())
				return detail::get_render_transaction().set(ptr->system_data, key, std::forward<T>(value));
			else
				return ptr->system_data.set(key, std::forward<T>(value));
		}
	}

	namespace render::level
	{
		template<typename T>
		curve<T> get_curve(variable_id v)
		{
			auto ptr = detail::get_render_data_ptr();
			assert(ptr);


			return curve<T>();
		}

		template<typename T>
		curve<T> get_curve(curve_index_t i)
		{
			auto ptr = detail::get_render_data_ptr();
			assert(ptr);

			auto &curves = ptr->level_data->get_curves();
			auto &curve_map = get_curve_list<T>(curves);

			if (detail::get_render_data_async())
				return detail::get_render_transaction().get(i, curve_map);
			else
				return curve_map.get(i);
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