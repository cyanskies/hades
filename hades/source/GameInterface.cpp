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

	void GameInterface::attachSystem(EntityId ent, data::UniqueId sys, sf::Time t)
	{

	}

	void GameInterface::detachSystem(EntityId ent, data::UniqueId sys, sf::Time t)
	{
	}
}