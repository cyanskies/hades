#ifndef HADES_GAMEINSTANCE_HPP
#define HADES_GAMEINSTANCE_HPP

#include "SFML/System/Time.hpp"

#include "Hades/ExportedCurves.hpp"
#include "Hades/GameInterface.hpp"
#include "Hades/level.hpp"
#include "Hades/systems.hpp"

namespace hades
{
	class variable_without_name : public std::logic_error
	{
		using std::logic_error::logic_error;
	};

	//represents the logic side of an individual level or game
	//TODO:
	// support saving
	// support loading
	class GameInstance : public GameInterface
	{
	public:
		GameInstance();
		GameInstance(level_save);

		//triggers all systems with the specified time change
		void tick(sf::Time dt);

		void insertInput(input_system::action_set input, sf::Time t);

		//exports all the newest keyframes so that they can be transmitted across the network
		//also sends the new variable id mappings
		//also sends entity name mappings
		//  t: send all keyframes after this point
		//		default is max time
		ExportedCurves getChanges(sf::Time t) const;
		//clears the new entity name list so the same list of entities don't get sent every tick
		//names are used for common entities to make finding them easy
		// master: the games master entity, used to store game rules, whether the game as ended yet
		// terrain: the terrain entity, all the world terrain will be stored as this entity's properties
		void nameEntity(EntityId entity, const types::string &name, sf::Time t);

	private:
		//the games starting time, is always 0
		const sf::Time _startTime = sf::Time();
		//the current time, this is the sum of all dt from the game ticks
		sf::Time _currentTime = _startTime;

		//diffs for the entitynames list
		std::vector<std::pair<EntityId, types::string>> _newEntityNames;

		curve<sf::Time, input_system::action_set> _input;
		//each tick will generate events that are handled at the end of that tick
		//events created by other events will be handled at the end of the next tick
	};
}

#endif //HADES_GAMEINSTANCE_HPP