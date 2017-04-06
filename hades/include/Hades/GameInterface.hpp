#ifndef HADES_GAMEINTERFACE_HPP
#define HADES_GAMEINTERFACE_HPP

#include <atomic>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <vector>

#include "SFML/System/Time.hpp"

#include "Hades/Curves.hpp"
#include "Hades/Types.hpp"
#include "Hades/value_guard.hpp"

namespace hades
{
	//we use int32 as the id type so that entity id's can be stored in curves.
	using EntityId = types::int32;
	//we do the same with variable Ids since they also need to be unique and easily network transferrable
	using VariableId = EntityId;

	const EntityId NO_ENTITY = std::numeric_limits<EntityId>::min();
	const VariableId NO_VARIABLE = std::numeric_limits<VariableId>::min();
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
		// they will also throw 'curve_error' if the curve is invalid
		// get a reference to the curve you need, and use it to modify the data
		//Int, is for storing positions, health, entityId's, and so on
		//Can be used for event pulses, just store the Id of the entity that contains the event data
		GameCurve<types::int32>* makeInt(EntityId, VariableId, CurveType);
		GameCurve<types::int32>* getInt(EntityId, VariableId) const;
		//Bool is used for flags, or for event pulses
		GameCurve<bool>* makeBool(EntityId, VariableId, CurveType);
		GameCurve<bool>* getBool(EntityId, VariableId) const;
		//string, not for dialogue, stuff like that should be stored in mod files
		//use it to store, unique names, should have less of these than ints and bools
		GameCurve<types::string>* makeString(EntityId, VariableId, CurveType);
		GameCurve<types::string>* getString(EntityId, VariableId) const;

		//helper to avoid template bloat with vector data in maps
		////NOTE: solves warning C4503
		template<class T>
		struct vector_data
		{
			std::vector<T> data;
		};

		//Vector versions of the other variable types
		GameCurve<vector_data<types::int32>>* makeIntVector(EntityId, VariableId, CurveType);
		GameCurve<vector_data<types::int32>>* getIntVector(EntityId, VariableId) const;

		GameCurve<vector_data<bool>>* makeBoolVector(EntityId, VariableId, CurveType);
		GameCurve<vector_data<bool>>* getBoolVector(EntityId, VariableId) const;

		GameCurve<vector_data<types::string>>* makeStringVector(EntityId, VariableId, CurveType);
		GameCurve<vector_data<types::string>>* getStringVector(EntityId, VariableId) const;

		//creates a variable name if it didn't already exist
		VariableId getVariableId(types::string);

		//TODO:
		//void attach system
		//void detach system

	protected:
		using EntityNameMap = std::map<types::string, EntityId>;

		//protected, since the ability to name entities is provided by a child.
		mutable std::shared_mutex EntNameMutex;
		EntityNameMap EntityNames;

		using VariableNameMap = std::map<types::string, VariableId>;
		//mapping of names to variable ids
		mutable std::shared_mutex VariableIdMutex;
		VariableNameMap VariableIds;
		std::atomic<VariableId> VariableNext = std::numeric_limits<VariableId>::min();

	private:
		std::atomic<EntityId> _next = std::numeric_limits<EntityId>::min();

		//CURVE VARIABLES
		//curve list
		//a curve has an entity id,that it's attached too, and a name
		template<class T>
		using CurveMap = std::map< std::pair<EntityId, VariableId>, std::unique_ptr<GameCurve<T>> >;

		mutable std::shared_mutex _intCurveMutex, _boolCurveMutex, _stringCurveMutex;
		CurveMap<types::int32> _intCurves;
		//no linear curves here
		CurveMap<bool> _boolCurves;
		CurveMap<types::string> _stringCurves;

		mutable std::shared_mutex _intVectMutex, _boolVectMutex, _stringVectMutex;
		//vector curves
		//no linear curves for these either
		CurveMap<vector_data<types::int32>> _intVectorCurves;
		CurveMap<vector_data<bool>> _boolVectorCurves;
		CurveMap<vector_data<types::string>> _stringVectorCurves;
	};
}

#endif //HADES_GAMEINTERFACE_HPP