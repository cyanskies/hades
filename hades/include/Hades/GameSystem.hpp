#ifndef HADES_GAMESYSTEM_HPP
#define HADES_GAMESYSTEM_HPP

#include <functional>
#include <vector>

#include "Hades/Input.hpp"
#include "Hades/parallel_jobs.hpp"
#include "Hades/simple_resources.hpp"
#include "Hades/value_guard.hpp"

namespace hades
{
	class GameInterface;

	//we use int32 as the id type so that entity id's can be stored in curves.
	using EntityId = types::int32;
	//TODO: variableId needs to be a uniqueId so that they can be syncronised across networks
	//this isn't needed for EntityId's and Entity names are strings, and rarely used, where
	//curves need to be identified often by a consistant lookup name
	//we do the same with variable Ids since they also need to be unique and easily network transferrable
	using VariableId = data::UniqueId;

	//these are earmarked as error values
	const EntityId NO_ENTITY = std::numeric_limits<EntityId>::min();
	const VariableId NO_VARIABLE = VariableId::Zero;

	//data needed to run a system job
	//the entities the system is operating on
	//a reference to the game interface
	//the current game time
	//this frames dt
	struct system_job_data : public parallel_jobs::job_data
	{
		//the entities this system id attached too
		std::vector<EntityId> entities;
		GameInterface* game_data;
		//the current time, and the time to advance too(t + dt)
		sf::Time current_time, dt;
		//the input over time for systems to look at
		const Curve<sf::Time, InputSystem::action_set> *actions;
	};

	//the interface for game systems.
	//systems work by creating jobs and passing along the data they will use.
	struct GameSystem
	{
		//this holds the systems, name and id, and the function that the system uses.
		resources::system* system;
		//list of entities attached to this system, over time
		value_guard<Curve<sf::Time, std::vector<EntityId>>> attached_entities = Curve<sf::Time, std::vector<EntityId>>(CurveType::STEP);
	};

	//program provided systems should be attatched to the renderer or 
	//gameinstance depending on what kind of system they are

	//scripted systems should be listed in the GameSystem: and RenderSystem: lists in
	//the mod files that added them
}

#endif //HADES_GAMESYSTEM_HPP