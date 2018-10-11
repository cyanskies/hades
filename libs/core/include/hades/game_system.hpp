#ifndef HADES_GAME_SYSTEM_HPP
#define HADES_GAME_SYSTEM_HPP

#include <any>
#include <functional>
#include <vector>

//#include "SFML/Graphics/Drawable.hpp"
//#include "SFML/Graphics/RenderStates.hpp"
//#include "SFML/System/Time.hpp"

#include "hades/curve_extra.hpp"
#include "hades/data.hpp"
#include "hades/game_types.hpp"
#include "hades/input.hpp"
#include "hades/parallel_jobs.hpp"
#include "hades/resource_base.hpp"
#include "hades/shared_guard.hpp"
#include "hades/timers.hpp"
//#include "Hades/SpriteBatch.hpp"

namespace hades
{
	//fwd declaration
	class GameInterface;
	class RenderInterface;

	class system_error : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	struct system_job_data
	{
		//entity to run on
		entity_id entity;
		//unique id of this system TODO: is this needed?
		unique_id system;
		//level data interface:
		// contains units, particles, buildings, terrain
		// per level quests and objectives
		GameInterface* level_data;
		//mission data interface
		// contains players, 
		// and... just the players
		GameInterface* mission_data;
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

			system_func on_create,
				on_connect,
				on_disconnect,
				tick,
				on_destroy;

			std::any system_info; //stores the system object or script reference.

			//if loaded from a manifest then it should be loaded from scripts
			//if it's provided by the application, then source is empty, and no laoder function is provided.

			//oncreate() //called when the system object is created
			//onconnect() //called when the system object is attached to an entity
			//ondisconnect() //called when detached from an entity
			//tick(currenttime, dt) //called once per update
			//ondestroy() //called when system object is being destroyed
			//on_event()
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
	struct GameSystem
	{
		GameSystem(const resources::system* s) : system(s)
		{}

		GameSystem(const resources::system* s, name_list nl) : system(s), attached_entities(std::move(nl))
		{}

		GameSystem(const GameSystem&) = default;
		GameSystem(GameSystem&&) = default;

		GameSystem &operator=(const GameSystem&) = default;
		GameSystem &operator=(GameSystem&&) = default;

		//this holds the systems, name and id, and the function that the system uses.
		const resources::system* system = nullptr;
		//list of entities attached to this system, over time
		shared_guard<name_list> attached_entities = name_list(curve_type::step);
	};

	//program provided systems should be attatched to the renderer or 
	//gameinstance depending on what kind of system they are

	//scripted systems should be listed in the GameSystem: and RenderSystem: lists in
	//the mod files that added them

	//TODO: add render systems here

	class RenderInterface;

	struct render_job_data
	{
		//entity to run on
		entity_id entity;
		//unique id of this system TODO: is this needed?
		unique_id system;
		//level data interface:
		// contains units, particles, buildings, terrain
		// per level quests and objectives
		RenderInterface* level_data;
		//mission data interface
		// contains players, 
		// and... just the players
		RenderInterface* mission_data;
		//the current time, and the time to advance too(t + dt)
		time_point current_time;
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

	struct RenderSystem
	{
		//this holds the systems, name and id, and the function that the system uses.
		const resources::render_system* system;
		//list of entities attached to this system, over time
		shared_guard<name_list> attached_entities = name_list{ curve_type::step };
	};
}

#include "hades/detail/game_system.inl"

#endif //HADES_GAMESYSTEM_HPP