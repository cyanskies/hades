#ifndef HADES_GAMEINSTANCE_HPP
#define HADES_GAMEINSTANCE_HPP

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>

#include "SFML/System/Time.hpp"

#include "Hades/Curves.hpp"
#include "Hades/Types.hpp"
#include "Hades/UniqueId.hpp"
#include "Hades/value_guard.hpp"

namespace hades
{
	//we use int32 as the id type so that entity id's can be stored in curves.
	using EntityId = types::int32;
	//we do the same with variable Ids since they also need to be unique and easily network transferrable
	using VariableId = EntityId;

	const EntityId NO_ENTITY = std::numeric_limits<EntityId>::min();
	//this is the interface that is available to jobs and systems
	//it supports multi threading the whole way though
	class GameInterface
	{
	public:
		virtual ~GameInterface() {}

		//creates an entity with no curves or systems attached to it
		EntityId createEntity();
		EntityId getEntityId(const types::string &name) const;

		template<class T>
		using GameCurve = value_guard< Curve<sf::Time, T> >;

		//NOTE: these functions will create curves if they don't already exist
		// they will also throw if the curve is invalid
		// get a reference to the curve you need, and use it to modify the data
		//Int, is for storing positions, health, entityId's, and so on
		//Can be used for event pulses, just store the Id of the entity that contains the event data
		GameCurve<types::int32>* makeCurveInt(EntityId, VariableId, CurveType);
		GameCurve<types::int32>* getInt(EntityId, VariableId) const;
		//Bool is used for flags, or for event pulses
		GameCurve<bool>* getBool(EntityId, VariableId) const;
		//string, not for dialogue, stuff like that should be stored in mod files
		//use it to store, unique names, should have less of these than ints and bools
		GameCurve<types::string>* getString(EntityId, VariableId) const;

		//Vector versions of the other variable types
		GameCurve<std::vector<types::int32>>* getIntVector(EntityId, VariableId) const;
		GameCurve<std::vector<bool>>* getBoolVector(EntityId, VariableId) const;
		GameCurve<std::vector<types::string>>* getStringVector(EntityId, VariableId) const;

		//creates a variable name if it didn't already exist
		VariableId getVariableId(types::string);

	protected:
		using EntityNameMap = std::map<types::string, EntityId>;

		//protected, since the ability to name entities is provided by a child.
		mutable std::shared_mutex _entNameMutex;
		EntityNameMap _entityNames;

	private:
		std::atomic<EntityId> _next = std::numeric_limits<EntityId>::min();

		//CURVE VARIABLES
		//curve list
		//a curve has an entity id,that it's attached too, and a name
		using CurveId = std::pair<EntityId, VariableId>;
		template<class T>
		using Curve_ptr = std::unique_ptr<GameCurve<T>>;
		template<class T>
		using CurveMap = std::map< CurveId, Curve_ptr<T> >;

		mutable std::shared_mutex _intCurveMutex, _boolCurveMutex, _stringCurveMutex;
		CurveMap<types::int32> _intCurves;
		//no linear curves here
		CurveMap<bool> _boolCurves;
		CurveMap<types::string> _stringCurves;

		mutable std::shared_mutex _intVectMutex, _boolVectMutex, _stringVectMutex;
		//vector curves
		//no linear curves for these either
		CurveMap<std::vector<types::int32>> _intVectorCurves;
		CurveMap<std::vector<bool>> _boolVectorCurves;
		CurveMap<std::vector<types::string>> _stringVectorCurves;

		using VariableNameMap = std::map<types::string, VariableId>;

		std::shared_mutex _variableIdMutex;
		//mapping of names to variable ids
		VariableNameMap _variableIds;

		std::atomic<VariableId> _variableNext = std::numeric_limits<EntityId>::min();
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
		void getChanges(sf::Time t);
		//gets all data as packets
		//this is useful for new players joining the game
		//doesn't include all the curve history, only currently active entities
		//and properties related to them
		void getAll() const;
		//returns all the game data, enough to rewatch the game from begining.
		void getAllHistory() const;

		//names are used for common entities to make finding them easy
		// master: the games master entity, used to store game rules, whether the game as ended yet
		// terrain: the terrain entity, all the world terrain will be stored as this entity's properties
		void nameEntity(EntityId entity, const types::string &name);
		
		types::string getVariableName(VariableId);

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

		//systems(systems run logic by added keyframes to curves(and adding frame predictions)
		//how to handle events?(events are stored as pulse curves, the system will have to check the curve to see if theirs an event it is interested in
	};
}

#endif //HADES_GAMEINSTANCE_HPP
