#ifndef HADES_GAMEINSTANCE_HPP
#define HADES_GAMEINSTANCE_HPP

#include "SFML/System/Time.hpp"

#include "Hades/GameInterface.hpp"
#include "Hades/GameSystem.hpp"

namespace hades
{
	//represents the logic side of an individual level or game
	//TODO:
	// support saving
	// support loading
	//non virutal functions might not be thread safe, and should not be used inside the job system
	class GameInstance : public GameInterface
	{
	public:
		//triggers all systems with the specified time change
		void tick(sf::Time dt);

		//exports all the newest keyframes so that they can be transmitted across the network
		//also sends the new variable id mappings
		//also sends entity name mappings
		//  t: how far back to go looking for keyframes?
		void getChanges(sf::Time t) const;
		//gets all data for saving
		void getAll() const;
		
		//names are used for common entities to make finding them easy
		// master: the games master entity, used to store game rules, whether the game as ended yet
		// terrain: the terrain entity, all the world terrain will be stored as this entity's properties
		void nameEntity(EntityId entity, const types::string &name);
		
		types::string getVariableName(VariableId) const;

		//systems
		//systems use their UniqueId as their key
		// since they don't need to be synced accross the network
		//void createSystem(System system, );
		//void attachSystem(EntityId, UniqueId);
		//void detachSystem(EntityId, UniqueId);

	private:
		//the games starting time, is always 0
		const sf::Time _startTime = sf::Time();
		//the current time, this is the sum of all dt from the game ticks
		sf::Time _currentTime = _startTime;

		//systems(systems run logic by adding keyframes to curves(and adding frame predictions)
		std::vector<GameSystem> _systems;

		//how to handle events?(events are stored as pulse curves, the system will have to check the curve to see if theirs an event it is interested in
	};
}

#endif //HADES_GAMEINSTANCE_HPP