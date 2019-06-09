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
			if constexpr (std::is_invocable_r_v<bool, Func, job_system&, JobData>)
			{
				return [f](job_system &j, JobData job_data)->bool {
					return std::invoke(f, j, std::move(job_data));
				};
			}
			else if constexpr (std::is_invocable_v<Func, job_system&, JobData>)
			{
				return [f](job_system &j, JobData job_data)->bool {
					std::invoke(f, j, std::move(job_data));
					return true;
				};
			}
			else if constexpr (std::is_invocable_r_v<bool, Func, JobData>)
			{
				return [f](job_system &j, JobData job_data)->bool {
					return std::invoke(f, std::move(job_data));
				};
			}
			else if constexpr (std::is_invocable_v<Func, JobData>)
			{
				return [f](job_system &j, JobData job_data)->bool {
					std::invoke(f, std::move(job_data));
					return true;
				};
			}
			else if constexpr (std::is_null_pointer_v<Func>)
				return f;
			else
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
		thread_local render_job_data* render_data_ptr = nullptr;
		thread_local transaction render_transaction;
		thread_local bool render_data_async = true;
	}

	namespace render
	{
		template<typename T>
		T get_system_value(unique_id key)
		{
			assert(detail::render_data_ptr);

			if (detail::render_data_async)
				return detail::render_transaction.get(detail::render_data_ptr->system_data, key);
			else
				return detail::render_data_ptr->system_data.get_no_async(key);
		}

		template<typename T>
		void set_system_value(unique_id, T&& value)
		{
			assert(detail::render_data_ptr);

			if (detail::render_data_async)
				return detail::render_data_ptr->system_data.get(key);
			else
				return detail::render_data_ptr->system_data.get_no_async(key);
		}
	}

	namespace render::level
	{
		template<typename T>
		T get_curve(curve_index_t i)
		{
			assert(detail::render_data_ptr);

			auto &curves = detail::render_data_ptr->level_data->get_curves();
			auto &curve_map = get_curve_list<T>(curves);

			if (detail::render_data_async)
				return detail::render_transaction.get(i, curve_map);
			else
				return curve_map.get(i);
		}

		template<typename T>
		void set_curve(curve_index_t i, T&& value)
		{
			assert(detail::render_data_ptr);

			auto& curves = detail::render_data_ptr->level_data->get_curves();
			auto& curve_map = get_curve_list<std::decay_t<T>>(curves);

			if (detail::render_data_async)
				return detail::render_transaction.set(curve_map, i, std::forward(value);
			else
				return curve_map.set(i, std::forward(value));
		}
	}
}