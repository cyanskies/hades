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

#include "hades/any_map.hpp"
#include "hades/curve.hpp"
#include "hades/shared_guard.hpp"
#include "hades/shared_map.hpp"
#include "Hades/simple_resources.hpp"
#include "Hades/systems.hpp"
#include "Hades/Types.hpp"

namespace hades
{
	struct curve_data
	{
		template<class T>
		using CurveMap = shared_map< std::pair<EntityId, VariableId>, curve<sf::Time, T> >;

		CurveMap<types::int32> intCurves;
		CurveMap<float> floatCurves;
		//no linear curves here
		CurveMap<bool> boolCurves;
		CurveMap<types::string> stringCurves;
		CurveMap<unique_id> uniqueCurves;

		//no linear curves here either
		CurveMap<std::vector<types::int32>> intVectorCurves;
		CurveMap <std::vector<float>> floatVectorCurves;
		CurveMap<std::vector<unique_id>> uniqueVectorCurves;
	};

	class system_already_attached : public std::runtime_error
	{
		using std::runtime_error::runtime_error;
	};

	class system_not_attached : public std::runtime_error
	{
		using std::runtime_error::runtime_error;
	};

	class curve_not_registered : public std::logic_error
	{
		using std::logic_error::logic_error;
	};

	using property_map = any_map<unique_id>;
	using name_curve_t = curve<sf::Time, std::map<types::string, EntityId>>;

	struct level_save;

	//this is the interface that is available to jobs and systems
	//it supports multi threading the whole way though
	class GameInterface
	{
	public:
		virtual ~GameInterface() {}

		GameInterface(const level_save&);

		//creates an entity with no curves or systems attached to it
		EntityId createEntity();
		EntityId getEntityId(const types::string &name, sf::Time t) const;

		curve_data &getCurves();
		const curve_data &getCurves() const;

		//const property_map &getProperties() const;

		//attach/detach entities from systems
		void attachSystem(EntityId, unique_id, sf::Time t);
		void detachSystem(EntityId, unique_id, sf::Time t);

	protected:
		//TODO: do these need to remain?
		GameSystem& FindSystem(unique_id);
		GameSystem& install_system(unique_id sys);

		shared_guard<name_curve_t> _entity_names = name_curve_t{ curve_type::step };
		mutable std::mutex _system_list_mut;
		std::vector<GameSystem> _systems;

		std::atomic<EntityId> _next = std::numeric_limits<EntityId>::min() + 1;
	private:
		//CURVE VARIABLES
		curve_data _curves;
		//shared properties
		//property_map _properties;
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
