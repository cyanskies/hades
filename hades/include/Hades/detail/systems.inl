#include "Hades/systems.hpp"

#include <type_traits>

#include "hades/data.hpp"
#include "hades/data_system.hpp"

namespace hades
{
	template<typename CreateFunc, typename ConnectFunc, typename DisconnectFunc, typename TickFunc, typename DestroyFunc>
	void make_system(unique_id id, CreateFunc on_create, ConnectFunc on_connect, DisconnectFunc on_disconnect, TickFunc on_tick, DestroyFunc on_destroy, data::data_manager &data)
	{
		using resources::system;

		//TODO: constexpr if using std::is_invocable to allow for different return values and not using job_system

		//assert that the Functions can be converted to the correct std::function type
		static_assert(std::is_constructible<system::system_func, CreateFunc>::value,
			"on_create must be a function object with the following definition: bool func(hades::job_system&, hades::system_job_data)");
		static_assert(std::is_constructible<system::system_func, ConnectFunc>::value,
			"on_connect must be a function object with the following definition: bool func(hades::job_system&, hades::system_job_data)");
		static_assert(std::is_constructible<system::system_func, DisconnectFunc>::value,
			"on_disconnect must be a function object with the following definition: bool func(hades::job_system&, hades::system_job_data)");
		static_assert(std::is_constructible<system::system_func, TickFunc>::value,
			"on_tick must be a function object with the following definition: bool func(hades::job_system&, hades::system_job_data)");
		static_assert(std::is_constructible<system::system_func, DestroyFunc>::value,
			"on_destroy must be a function object with the following definition: bool func(hades::job_system&, hades::system_job_data)");

		auto sys = data::FindOrCreate<system>(id, unique_id::zero, &data);

		//TODO: correct exception here
		if (!sys)
			throw std::exception("unable to create requested system");

		sys->on_create = on_create;
		sys->on_connect = on_connect;
		sys->on_disconnect = on_disconnect;
		sys->tick = on_tick;
		sys->on_destroy = on_destroy;
	}
}