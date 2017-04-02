#include "Hades/GameInstance.hpp"

namespace hades 
{
	void GameInstance::tick(sf::Time dt)
	{
		//run all systems on all attached entities
	}

	void GameInstance::getChanges(sf::Time t) const
	{
		//return all frames after 't'
	}

	void GameInstance::getAll() const
	{
		//return all frames
	}

	void GameInstance::nameEntity(EntityId entity, const types::string &name)
	{
		std::unique_lock<std::shared_mutex> lk(_entNameMutex);
		_entityNames[name] = entity;
	}

	types::string GameInstance::getVariableName(VariableId id) const
	{
		std::shared_lock<std::shared_mutex> lk(_variableIdMutex);
		for (auto n : _variableIds)
		{
			if (n.second == id)
				return n.first;
		}

		//TODO: throw instead;
		return types::string();
	}
}