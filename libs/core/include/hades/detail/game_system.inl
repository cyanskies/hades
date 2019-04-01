#include "hades/game_system.hpp"

#include <type_traits>

#include "hades/data.hpp"

namespace hades
{
	namespace detail
	{
		template<typename JobData, typename Func>
		inline auto make_system_func(Func f)
		{
			if constexpr (std::is_invocable_r_v<bool, Func, job_system&, system_job_data>)
			{
				return [f](job_system &j, JobData job_data)->bool {
					return std::invoke(f, j, std::move(job_data));
				};
			}
			else if constexpr (std::is_invocable_v<Func, job_system&, system_job_data>)
			{
				return [f](job_system &j, JobData job_data)->bool {
					std::invoke(f, j, std::move(job_data));
					return true;
				};
			}
			else if constexpr (std::is_invocable_r_v<bool, Func, system_job_data>)
			{
				return [f](job_system &j, JobData job_data)->bool {
					return std::invoke(f, std::move(job_data));
				};
			}
			else if constexpr (std::is_invocable_v<Func, system_job_data>)
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
}