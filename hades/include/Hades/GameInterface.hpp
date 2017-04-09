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
#include "Hades/simple_resources.hpp"
#include "Hades/Types.hpp"
#include "Hades/transactional_map.hpp"

namespace hades
{
	//we use int32 as the id type so that entity id's can be stored in curves.
	using EntityId = types::int32;
	//we do the same with variable Ids since they also need to be unique and easily network transferrable
	using VariableId = EntityId;

	const EntityId NO_ENTITY = std::numeric_limits<EntityId>::min();
	const VariableId NO_VARIABLE = std::numeric_limits<VariableId>::min();

	struct curve_data
	{
		template<class T>
		using CurveMap = transactional_map< std::pair<EntityId, VariableId>, Curve<sf::Time, T> >;

		CurveMap<types::int32> intCurves;
		//no linear curves here
		CurveMap<bool> boolCurves;
		CurveMap<types::string> stringCurves;

		//no linear curves here either
		CurveMap<std::vector<types::int32>> intVectorCurves;
		CurveMap<std::vector<bool>> boolVectorCurves;
		CurveMap<std::vector<types::string>> stringVectorCurves;
	};
	//this is the interface that is available to jobs and systems
	//it supports multi threading the whole way though
	class GameInterface
	{
	public:
		virtual ~GameInterface() {}

		//creates an entity with no curves or systems attached to it
		EntityId createEntity();
		EntityId getEntityId(const types::string &name) const;

		curve_data &getCurves();

		//creates a variable name if it didn't already exist
		VariableId getVariableId(types::string);

		void createSystem(resources::system* system);

		//TODO:
		//void attach system to entity
		//void detach system from entity

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
		curve_data _curves;
	};
}

#endif //HADES_GAMEINTERFACE_HPP