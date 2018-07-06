#include "Hades/GameInterface.hpp"

#include "Hades/data.hpp"
#include "Hades/simple_resources.hpp"

namespace hades 
{
	EntityId GameInterface::createEntity()
	{
		return ++_next;
	}

	EntityId GameInterface::getEntityId(const types::string &name, sf::Time t) const
	{
		const auto names = _entity_names.get();
		const auto name_map = names.get(t);
		if (const auto ent_name = name_map.find(name); ent_name != std::end(name_map))
			return ent_name->second;

		return NO_ENTITY;
	}

	curve_data &GameInterface::getCurves()
	{
		return _curves;
	}

	const curve_data &GameInterface::getCurves() const
	{
		return _curves;
	}

	void GameInterface::attachSystem(EntityId entity, unique_id sys, sf::Time t)
	{
		std::shared_lock<std::shared_mutex> lk;
		GameSystem* system;

		std::tie(lk, system) = FindSystem(sys);

		if(!system)
			throw system_already_attached(std::string("Cannot attach to system: err, bacause the system is not part of this instance").c_str());

		bool try_again = true;
		auto &ents = system->attached_entities;

		//keep tring to modify the entity list untill it works
		//this is a compare exchange loop
		while (try_again)
		{
			auto oldcurve = ents.load();
			auto newcurve = oldcurve;

			auto value = newcurve.get(t);
			auto found = std::find(value.begin(), value.end(), entity);
			if (found != value.end())
			{
				//ent is already attached
				throw system_already_attached(std::string("The requested entityid is already attached to this system. EntityId: " \
					+ std::to_string(entity) + ", System: " + "err" + ", at time: " + std::to_string(t.asSeconds()) + "s").c_str());
			}

			//insert
			value.push_back(entity);
			newcurve.set(t, value);

			//insert back into the value guard
			//if this fails, it means the value has changed since we retrieved it
			//so loop, grab the value again and try again
			if (ents.compare_exhange(oldcurve, newcurve))
				return;
		}//!while try_again
	}

	void GameInterface::detachSystem(EntityId entity, unique_id sys, sf::Time t)
	{
		std::shared_lock<std::shared_mutex> lk;
		GameSystem* system;

		std::tie(lk, system) = FindSystem(sys);

		if(!system)
			throw system_not_attached(std::string("Cannot detach from system: err, bacause the system is not part of this instance").c_str());

		bool try_again = true;
		auto &ents = system->attached_entities;

		while (try_again)
		{
			auto oldcurve = ents.load();
			auto newcurve = oldcurve;

			auto value = newcurve.get(t);
			auto found = std::find(value.begin(), value.end(), entity);
			if (found == value.end())
			{
				//ent is already attached
				throw system_not_attached(std::string("The requested entityid is not attached to this system. EntityId: " \
					+ std::to_string(entity) + ", System: " + "err" + ", at time: " + std::to_string(t.asSeconds()) + "s").c_str());
			}

			//insert
			value.erase(found);
			newcurve.set(t, value);

			//insert back into the value guard
			//if this fails, it means the value has changed since we retrieved it
			//so loop, grab the value again and try again
			if (ents.compare_exhange(oldcurve, newcurve))
				return;
		} //!while modify the ent list
	}

	void GameInterface::install_system(unique_id sys)
	{
		const auto system_list = _systems.get();

		//never install a system more than once.
		assert(std::none_of(std::begin(system_list), std::end(system_list), [sys](const auto &system) {
			return system.system->id == sys;
		}));

		auto updated_list = system_list;

		const auto new_system = hades::data::get<resources::system>(sys);

		updated_list.push_back(GameSystem{ new_system });
	}
}