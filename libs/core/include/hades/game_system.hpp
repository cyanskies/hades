#ifndef HADES_GAME_SYSTEM_HPP
#define HADES_GAME_SYSTEM_HPP

#include <any>
#include <functional>
#include <vector>

#include "hades/curve_extra.hpp"
#include "hades/data.hpp"
#include "hades/exceptions.hpp"
#include "hades/game_types.hpp"
#include "hades/input.hpp"
#include "hades/parallel_jobs.hpp"
#include "hades/resource_base.hpp"
#include "hades/shared_guard.hpp"
#include "hades/timers.hpp"

namespace hades
{
	//fwd declaration
	class game_interface;
	
	class system_error : public runtime_error
	{
	public:
		using runtime_error::runtime_error;
	};

	struct system_job_data
	{
		//entity to run on
		entity_id entity = bad_entity;
		//unique id of this system TODO: is this needed?
		unique_id system = unique_id::zero;
		//level data interface:
		// contains units, particles, buildings, terrain
		// per level quests and objectives
		game_interface *level_data = nullptr;
		//TODO: map of uniqiue id to other game interface ads
		//mission data interface
		// contains players, 
		// and... just the players
		game_interface *mission_data = nullptr;
		//the current time, and the time to advance by(t + dt)
		time_point current_time;
		time_duration dt;
	};

	namespace resources
	{
		struct system_t
		{};

		//a system stores a job function
		struct system : public resource_type<system_t>
		{
			using system_func = std::function<bool(job_system&, system_job_data)>;

			system_func on_create, //called on system creation(or large time leap)
				on_connect,			//called when attached to an entity(or large time leap)
				on_disconnect,     //called when detatched from ent
				tick,				//called every tick
				on_destroy;			//called on system destruction(or large time leap, before on_* functions)
			//	on_event?

			std::any system_info; //stores the system object or script reference.
			//if loaded from a manifest then it should be loaded from scripts
			//if it's provided by the application, then source is empty, and no laoder function is provided.
		};
	}

	template<typename CreateFunc, typename ConnectFunc, typename DisconnectFunc, typename TickFunc, typename DestroyFunc>
	void make_system(unique_id id, CreateFunc on_create, ConnectFunc on_connect, DisconnectFunc on_disconnect, TickFunc on_tick, DestroyFunc on_destroy, data::data_manager&);

	template<typename CreateFunc, typename ConnectFunc, typename DisconnectFunc, typename TickFunc, typename DestroyFunc>
	void make_system(std::string_view name, CreateFunc on_create, ConnectFunc on_connect, DisconnectFunc on_disconnect, TickFunc on_tick, DestroyFunc on_destroy, data::data_manager &data)
	{
		return make_system(data.get_uid(name), on_create, on_connect, on_disconnect, on_tick, on_destroy, data);
	}

	//this isn't needed for EntityId's and Entity names are strings, and rarely used, where
	//curves need to be identified often by a consistant lookup name
	//we do the same with variable Ids since they also need to be unique and easily network transferrable
	using variable_id = unique_id;
	const variable_id bad_variable = variable_id::zero;

	using name_list = curve<resources::curve_types::vector_object_ref>;

	//the interface for game systems.
	//systems work by creating jobs and passing along the data they will use.
	struct game_system
	{
		using system_t = resources::system;
		using job_data_t = system_job_data;

		explicit game_system(const resources::system* s) : system(s)
		{}

		game_system(const resources::system* s, name_list nl) : system{ s }, attached_entities{ std::move(nl) }
		{}

		game_system(const game_system&) = default;
		game_system(game_system&&) = default;

		game_system &operator=(const game_system&) = default;
		game_system &operator=(game_system&&) = default;

		//this holds the systems, name and id, and the function that the system uses.
		const resources::system* system = nullptr;
		//list of entities attached to this system, over time
		shared_guard<name_list> attached_entities = name_list(curve_type::step);
	};

	//program provided systems should be attatched to the renderer or 
	//gameinstance depending on what kind of system they are

	//scripted systems should be listed in the game_system: and render_system: lists in
	//the mod files that added them

	class render_interface;

	struct render_job_data
	{
		//entity to run on
		entity_id entity = bad_entity;
		//unique id of this system TODO: is this needed?
		unique_id system = unique_id::zero;
		//level data interface:
		// contains units, particles, buildings, terrain
		// per level quests and objectives
		game_interface *level_data = nullptr;
		//mission data interface
		// contains players, 
		// and... just the players
		game_interface *mission_data = nullptr;
		//the current time, and the time to advance too(t + dt)
		time_point current_time;
		//do we need dt for the renderer?

		//render output interface
		render_interface *render_output = nullptr;

		//the input over time for systems to look at TODO: find another way to do this
		//const curve<sf::Time, input_system::action_set> *actions;
	};

	namespace resources
	{
		struct render_system_t
		{};

		struct render_system : public resource_type<render_system_t>
		{
			using system_func = std::function<bool(job_system&, render_job_data)>;

			system_func on_create,
				on_connect,
				on_disconnect,
				tick,
				on_destroy;

			std::any system_info;
		};
	}

	struct render_system
	{
		using system_t = resources::render_system;
		using job_data_t = render_job_data;

		explicit render_system(const resources::render_system* s) : system(s)
		{}

		render_system(const resources::render_system* s, name_list nl) : system{ s }, attached_entities{ std::move(nl) }
		{}

		render_system(const render_system&) = default;
		render_system(render_system&&) = default;

		render_system& operator=(const render_system&) = default;
		render_system& operator=(render_system&&) = default;

		//this holds the systems, name and id, and the function that the system uses.
		const resources::render_system *system = nullptr;
		//list of entities attached to this system, over time
		shared_guard<name_list> attached_entities = name_list{ curve_type::step };
	};

	template<typename CreateFunc, typename ConnectFunc, typename DisconnectFunc, typename TickFunc, typename DestroyFunc>
	void make_render_system(unique_id id, CreateFunc on_create, ConnectFunc on_connect, DisconnectFunc on_disconnect, TickFunc on_tick, DestroyFunc on_destroy, data::data_manager&);
	
	template<typename CreateFunc, typename ConnectFunc, typename DisconnectFunc, typename TickFunc, typename DestroyFunc>
	void make_render_system(std::string_view id, CreateFunc on_create, ConnectFunc on_connect, DisconnectFunc on_disconnect, TickFunc on_tick, DestroyFunc on_destroy, data::data_manager &d)
	{
		make_render_system(d.get_uid(id), on_create, on_connect, on_disconnect, on_tick, on_destroy, d);
	}
}

#include "hades/detail/game_system.inl"

#endif //!HADES_GAMESYSTEM_HPP