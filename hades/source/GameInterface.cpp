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

	template<class T, class mut>
	GameInterface::GameCurve<T>* makeCurve(EntityId ent, VariableId var, CurveType type,
		GameInterface::CurveMap<T> &curves, mut &mutex)
	{
		std::lock_guard<mut> lk(mutex);
		auto &c_ptr = curves[{ent, var}];
		//if the curve already exists then error
		if(c_ptr != nullptr)
			throw curve_error(std::string("Tried to create curve for ent: " + std::to_string(ent) + ", var: " + std::to_string(var) + ", that already exists.").c_str());

		//create curve and return
		c_ptr = std::make_unique<GameInterface::GameCurve<T>>(type);
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
			throw curve_error(std::string("Requested curve, EntId: " + std::to_string(ent) + ", Var: " + std::to_string(var) + ", that doesn't exist, or is the wrong type").c_str());
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

	GameInterface::GameCurve<GameInterface::vector_data<types::int32>>* GameInterface::makeIntVector(EntityId ent, VariableId var, CurveType t)
	{
		return makeCurve(ent, var, t, _intVectorCurves, _intVectMutex);
	}

	GameInterface::GameCurve<GameInterface::vector_data<types::int32>>* GameInterface::getIntVector(EntityId ent, VariableId var) const
	{
		return getCurve(ent, var, _intVectorCurves, _intVectMutex);
	}

	GameInterface::GameCurve<GameInterface::vector_data<bool>>* GameInterface::makeBoolVector(EntityId ent, VariableId var, CurveType t)
	{
		return makeCurve(ent, var, t, _boolVectorCurves, _boolVectMutex);
	}

	GameInterface::GameCurve<GameInterface::vector_data<bool>>* GameInterface::getBoolVector(EntityId ent, VariableId var) const
	{
		return getCurve(ent, var, _boolVectorCurves, _boolVectMutex);
	}

	GameInterface::GameCurve<GameInterface::vector_data<types::string>>* GameInterface::makeStringVector(EntityId ent, VariableId var, CurveType t)
	{
		return makeCurve(ent, var, t, _stringVectorCurves, _stringVectMutex);
	}

	GameInterface::GameCurve<GameInterface::vector_data<types::string>>* GameInterface::getStringVector(EntityId ent, VariableId var) const
	{
		return getCurve(ent, var, _stringVectorCurves, _stringVectMutex);
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
}