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


	VariableId GameInterface::getVariableId(types::string s)
	{
		//try to get the value in shared mode
		{
			std::shared_lock<std::shared_mutex> sharedlk(VariableIdMutex);
			auto v = VariableIds.find(s);
			if (v != VariableIds.end())
				return v->second;
		}

		//if it doesn't exist take a unique lock and create it
		std::lock_guard<std::shared_mutex> lk(VariableIdMutex);
		return VariableIds[s] = ++VariableNext;
	}

	void GameInterface::attachSystem(EntityId entity, data::UniqueId sys, sf::Time t)
	{
		std::shared_lock<std::shared_mutex> lk(SystemsMutex);
		
		auto iter = Systems.begin();
		while (iter != Systems.end())
		{
			if (iter->system->id == sys)
			{
				bool try_again = true;
				auto &ents = iter->attached_entities;

				while (try_again)
				{
					auto oldcurve = ents.get();
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
					//if (ents.compare_exhange(oldcurve, newcurve))
						return;
				} //!while modify the ent list
			}//!if iter* == sys

			++iter;
		}//!while(iter++ != list.end)

		//system not found
		//error
		throw system_already_attached(std::string("Cannot attach to system: err, bacause the system is not part of this instance").c_str());
	}

	void GameInterface::detachSystem(EntityId ent, data::UniqueId sys, sf::Time t)
	{
		std::shared_lock<std::shared_mutex> lk(SystemsMutex);
	}
}