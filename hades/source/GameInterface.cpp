#include "Hades/GameInterface.hpp"

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
	GameInterface::GameCurve<T>* makeCurve(EntityId ent, VariableId var, CurveType type,
		GameInterface::CurveMap<T> &curves, mut &mutex)
	{
		std::unique_lock<mut> lk(mutex);
		auto &c_ptr = curves[{ent, var}];
		//if the curve already exists then error
		if(c_ptr != nullptr)
			throw curve_error("Tried to create curve for ent: " + ent + ", var: " + var + ", that already exists.");

		//create curve and return
		c_ptr = std::make_unique<Curve>(type);
		return &*c_ptr;
	}

	template<class T, class mut>
	GameInterface::GameCurve<T>* getCurve(EntityId ent, VariableId var, 
		const GameInterface::CurveMap<T> &curves, mut &mutex)
	{
		std::shared_lock<mut> lk(mutex);

		auto c = curves.find({ ent,var });

		if (c != curves.end())
			return &*c->second;
		else
		{
			throw curve_error("Requested curve, EntId: " + ent + ", Var: " + var + ", that doesn't exist, or is the wrong type");
		}
	}

	GameInterface::GameCurve<types::int32>* GameInterface::makeInt(EntityId ent, VariableId var, CurveType t)
	{
		return makeCurve(ent, var, t, _intCurves, _intCurveMutex);
	}

	GameInterface::GameCurve<types::int32>* GameInterface::getInt(EntityId ent, VariableId var) const
	{
		return getCurve(ent, var, _intCurves, _intCurveMutex);
	}

	GameInterface::GameCurve<bool>* GameInterface::makeBool(EntityId ent, VariableId var, CurveType t)
	{
		return makeCurve(ent, var, t, _boolCurves, _boolCurveMutex);
	}

	GameInterface::GameCurve<bool>* GameInterface::getBool(EntityId ent, VariableId var) const
	{
		return getCurve(ent, var, _boolCurves, _boolCurveMutex);
	}

	GameInterface::GameCurve<types::string>* GameInterface::makeString(EntityId ent, VariableId var, CurveType t)
	{
		return makeCurve(ent, var, t, _stringCurves, _stringCurveMutex);
	}

	GameInterface::GameCurve<types::string>* GameInterface::getString(EntityId ent, VariableId var) const
	{
		return getCurve(ent, var, _stringCurves, _stringCurveMutex);
	}

	GameInterface::GameCurve<std::vector<types::int32>>* GameInterface::makeIntVector(EntityId ent, VariableId var, CurveType t)
	{
		return makeCurve(ent, var, t, _intVectorCurves, _intVectMutex);
	}

	GameInterface::GameCurve<std::vector<types::int32>>* GameInterface::getIntVector(EntityId ent, VariableId var) const
	{
		return getCurve(ent, var, _intVectorCurves, _intVectMutex);
	}

}