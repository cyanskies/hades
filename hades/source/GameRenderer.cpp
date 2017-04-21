#include "Hades/GameRenderer.hpp"

#include <algorithm>

#include "Hades/DataManager.hpp"

namespace hades
{
	typename RendererInterface::SpriteMap &RendererInterface::getSprites()
	{
		return _sprites;
	}

	void GameRenderer::setTime(sf::Time time)
	{
		_currentTime = time;
	}

	void GameRenderer::addTime(sf::Time dt)
	{
		_currentTime += dt;
	}

	template<typename T>
	void ImportSet(typename curve_data::CurveMap<T> &target, const std::vector < ExportedCurves::ExportSet<T>> &set)
	{
		for (auto &s : set)
		{
			auto c = nc = target.get({ s.entity, s.variable });

			std::sort(s.frames.begin(), s.frames.end());
			for (auto &kf : s.frames)
				nc.set(kf.t, kf.value);
		}
	}

	void GameRenderer::insertFrames(ExportedCurves import)
	{
		std::unique_lock<std::shared_mutex> ent_name_lock(EntNameMutex);

		for (auto &e : import.entity_names)
			EntityNames.insert({ e.second, e.first });

		ent_name_lock.unlock();

		//TODO: variables shouldn't be synced along with the frames,
		// they should just be synced once on connection
		std::unique_lock<std::shared_mutex> var_name_lock(VariableIdMutex);
		for (auto v : import.variable_names)
			VariableIds.insert({data_manager->getUid(v.second), v.first });
		var_name_lock.unlock();

		auto &curves = getCurves();
		ImportSet(curves.boolCurves, import.boolCurves);
		ImportSet(curves.intCurves, import.intCurves);
		ImportSet(curves.intVectorCurves, import.intVectorCurves);
		ImportSet(curves.stringCurves, import.stringCurves);
	}

	void GameRenderer::installSystem(resources::system *system)
	{
		InstallSystem(system, SystemsMutex, Systems);
	}
}
