#ifndef HADES_GAMEINSTANCE_HPP
#define HADES_GAMEINSTANCE_HPP

#include "SFML/System/Time.hpp"

#include "Hades/ExportedCurves.hpp"
#include "Hades/GameInterface.hpp"
#include "Hades/GameSystem.hpp"

namespace hades
{
	class variable_without_name : public std::logic_error
	{
		using std::logic_error::logic_error;
	};

	class system_already_installed : public std::logic_error
	{
		using std::logic_error::logic_error;
	};

	class system_null : public std::logic_error
	{
		using std::logic_error::logic_error;
	};

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
		//		default is max time
		ExportedCurves getChanges(sf::Time t = sf::microseconds(std::numeric_limits<sf::Int64>::max())) const;

		//names are used for common entities to make finding them easy
		// master: the games master entity, used to store game rules, whether the game as ended yet
		// terrain: the terrain entity, all the world terrain will be stored as this entity's properties
		void nameEntity(EntityId entity, const types::string &name);
		
		void registerVariable(data::UniqueId);
		types::string getVariableName(VariableId) const;

		//adds a system to this instance.,
		//systems should be added as soon as possible
		void installSystem(resources::system *system);
	private:
		//the games starting time, is always 0
		const sf::Time _startTime = sf::Time();
		//the current time, this is the sum of all dt from the game ticks
		sf::Time _currentTime = _startTime;
		//how to handle events?(events are stored as pulse curves, the system will have to check the curve to see if theirs an event it is interested in
	};
}

#endif //HADES_GAMEINSTANCE_HPP