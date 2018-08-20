#ifndef HADES_SYSTEMS_HPP
#define HADES_SYSTEMS_HPP

#include <any>
#include <functional>
#include <vector>

#include "SFML/System/Time.hpp"

#include "hades/curve.hpp"
#include "hades/input.hpp"
#include "hades/parallel_jobs.hpp"
#include "hades/resource_base.hpp"
#include "Hades/shared_guard.hpp"

namespace hades
{
	class GameInterface;
	class RenderInterface;

	//we use int32 as the id type so that entity id's can be stored in curves.
	using EntityId = types::int32;

	struct system_job_data
	{
		//entity to run on
		EntityId entity;
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
		//the current time, and the time to advance too(t + dt)
		sf::Time current_time, dt;
		//the input over time for systems to look at TODO: find another way to do this
		//const curve<sf::Time, input_system::action_set> *actions;
	};

	struct render_job_data
	{
		//entity to run on
		EntityId entity;
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
		sf::Time current_time, dt;
		//the input over time for systems to look at TODO: find another way to do this
		//const curve<sf::Time, input_system::action_set> *actions;
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

	//TODO: variableId needs to be a uniqueId so that they can be syncronised across networks
	//this isn't needed for EntityId's and Entity names are strings, and rarely used, where
	//curves need to be identified often by a consistant lookup name
	//we do the same with variable Ids since they also need to be unique and easily network transferrable
	using VariableId = unique_id;

	//these are earmarked as error values
	const EntityId NO_ENTITY = std::numeric_limits<EntityId>::min();
	const VariableId NO_VARIABLE = VariableId::zero;

	//the interface for game systems.
	//systems work by creating jobs and passing along the data they will use.
	struct GameSystem
	{
		//this holds the systems, name and id, and the function that the system uses.
		const resources::system* system;
		//list of entities attached to this system, over time
		shared_guard<curve<sf::Time, std::vector<EntityId>>> attached_entities = curve<sf::Time, std::vector<EntityId>>(curve_type::step);

		//systems functions
		bool on_create(job_system&, system_job_data);
		bool on_connect(job_system&, system_job_data);
		bool on_disconnect(job_system&, system_job_data);
		bool on_tick(job_system&, system_job_data);
		bool on_destroy(job_system&, system_job_data);
	};

	//program provided systems should be attatched to the renderer or 
	//gameinstance depending on what kind of system they are

	//scripted systems should be listed in the GameSystem: and RenderSystem: lists in
	//the mod files that added them

	//TODO: add render systems here
}

#endif //HADES_GAMESYSTEM_HPP