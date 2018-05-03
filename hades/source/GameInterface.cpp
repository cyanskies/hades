#include "Hades/GameInterface.hpp"

namespace hades 
{
	EntityId GameInterface::createEntity()
	{
		return ++_next;
	}

	EntityId GameInterface::getEntityId(const types::string &name) const
	{
		std::shared_lock<std::shared_mutex> lk(EntNameMutex);
		auto ename = EntityNames.find(name);
		if (ename != EntityNames.end())
			return ename->second;

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

	std::tuple<std::shared_lock<std::shared_mutex>, GameSystem*> GameInterface::FindSystem(unique_id sys)
	{
		std::shared_lock<std::shared_mutex> lk(SystemsMutex);
		GameSystem* system = nullptr;

		auto iter = Systems.begin();
		while (iter != Systems.end())
		{
			if (iter->system->id == sys)
			{
				system = &*iter;
				break;
			}//!if iter* == sys
			++iter;
		}//!while(iter++ != list.end)

		return std::make_tuple(std::move(lk), system);
	}

	void InstallSystem(resources::system *system, std::shared_mutex &mutex, std::vector<GameSystem> &systems)
	{
		assert(system);

		std::lock_guard<std::shared_mutex> system_lock(mutex);

		if (!system)
			throw system_null("system pointer passed to GameInstance::installSystem was null");

		auto id = system->id;

		for (auto s : systems)
		{
			if (s.system->id == id)
				throw system_already_installed("tried to install system: <name>; that is already installed");
		}

		GameSystem s;
		s.system = system;
		curve<sf::Time, std::vector<EntityId>> entities(curve_type::step);
		entities.insert(sf::Time::Zero, {});
		s.attached_entities = entities;
	}
}