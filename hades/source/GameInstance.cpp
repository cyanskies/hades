#include "Hades/GameInstance.hpp"

namespace hades 
{
	EntityId GameInterface::createEntity()
	{
		return ++_next;
	}

	EntityId GameInterface::getEntityId(const types::string &name) const
	{
		std::shared_lock<std::shared_mutex> lk(_entNameMutex);
		auto ename = _entityNames.find(name);
		if (ename != _entityNames.end())
			return ename->second;

		return NO_ENTITY;
	}

	template<class T, class mut>
	GameInterface::GameCurve<T>* GameInterface::makeCurve(EntityId ent, VariableId var, CurveType type,
		GameInterface::CurveMap<T> &curves, mut &mutex)
	{
		std::unique_lock<mut> lk(mutex);

		//create curve and return
		return nullptr;
	}

	template<class T, class mut>
	GameInterface::GameCurve<T>* GameInterface::getCurve(EntityId ent, VariableId var, 
		const GameInterface::CurveMap<T> &curves, mut &mutex)
	{
		std::shared_lock<mut> lk(mutex);

		//check if curve exists, if so return it

		//if not, throw
		return nullptr;
	}

	GameInterface::GameCurve<types::int32>* GameInterface::makeCurveInt(EntityId, VariableId, CurveType)
	{
		return nullptr;
	}

	GameInterface::GameCurve<types::int32>* GameInterface::getInt(EntityId ent, VariableId var) const
	{
		return getCurve(ent, var, _intCurves, _intCurveMutex);
	}

}