#include "Hades/systems.hpp"

#include <type_traits>

#include "hades/data.hpp"
#include "hades/data_system.hpp"

namespace hades
{
	template<typename Func>
	auto make_system_func(Func f)
	{
		if constexpr (std::is_invocable_r_v<bool, Func, job_system&, system_job_data>)
		{
			return [f](job_system &j, system_job_data job_data)->bool {
				return std::invoke(f, j, std::move(job_data));
			};
		}
		else if constexpr (std::is_invocable_v<Func, job_system&, system_job_data>)
		{
			return [f](job_system &j, system_job_data job_data)->bool {
				std::invoke(f, j, std::move(job_data));
				return true;
			};
		}
		else if constexpr (std::is_invocable_r_v<bool, Func, system_job_data>)
		{
			return [f](job_system &j, system_job_data job_data)->bool {
				return std::invoke(f, std::move(job_data));
			};
		}
		else if constexpr (std::is_invocable_v<Func, system_job_data>)
		{
			return [f](job_system &j, system_job_data job_data)->bool {
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

	template<typename CreateFunc, typename ConnectFunc, typename DisconnectFunc, typename TickFunc, typename DestroyFunc>
	void make_system(unique_id id, CreateFunc on_create, ConnectFunc on_connect, DisconnectFunc on_disconnect, TickFunc on_tick, DestroyFunc on_destroy, data::data_manager &data)
	{
		using resources::system;

		auto sys = data::FindOrCreate<system>(id, unique_id::zero, &data);

		if (!sys)
			throw system_error("unable to create requested system");

		sys->on_create		= make_system_func(on_create);
		sys->on_connect		= make_system_func(on_connect);
		sys->on_disconnect = make_system_func(on_disconnect);
		sys->tick			= make_system_func(on_tick);
		sys->on_destroy		= make_system_func(on_destroy);
	}
}