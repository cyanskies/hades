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
#include "Hades/GameSystem.hpp"
#include "Hades/simple_resources.hpp"
#include "Hades/Types.hpp"
#include "Hades/transactional_map.hpp"

namespace hades
{
	struct curve_data
	{
		template<class T>
		using CurveMap = transactional_map< std::pair<EntityId, VariableId>, Curve<sf::Time, T> >;

		CurveMap<types::int32> intCurves;
		CurveMap<float> floatCurves;
		//no linear curves here
		CurveMap<bool> boolCurves;
		CurveMap<types::string> stringCurves;

		//no linear curves here either
		CurveMap<std::vector<types::int32>> intVectorCurves;
		CurveMap <std::vector<float>> floatVectorCurves;
	};

	class system_already_attached : public std::exception
	{
		using std::exception::exception;
	};

	class system_not_attached : public std::exception
	{
		using std::exception::exception;
	};

	class curve_not_registered : public std::logic_error
	{
		using std::logic_error::logic_error;
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
		const curve_data &getCurves() const;

		//attach/detach entities from systems
		void attachSystem(EntityId, data::UniqueId, sf::Time t);
		void detachSystem(EntityId, data::UniqueId, sf::Time t);

	protected:
		std::tuple<std::shared_lock<std::shared_mutex>, GameSystem*> FindSystem(data::UniqueId);

		using EntityNameMap = std::map<types::string, EntityId>;

		//protected, since the ability to name entities is provided by a child.
		mutable std::shared_mutex EntNameMutex;
		EntityNameMap EntityNames;

		mutable std::shared_mutex SystemsMutex;
		std::vector<GameSystem> Systems;

	private:
		std::atomic<EntityId> _next = std::numeric_limits<EntityId>::min() + 1;

		//CURVE VARIABLES
		curve_data _curves;
	};

	class system_already_installed : public std::logic_error
	{
		using std::logic_error::logic_error;
	};

	class system_null : public std::logic_error
	{
		using std::logic_error::logic_error;
	};

	void InstallSystem(resources::system*, std::shared_mutex&, std::vector<GameSystem>&);
}

#endif //HADES_GAMEINTERFACE_HPP