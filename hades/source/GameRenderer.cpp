#include "Hades/GameRenderer.hpp"

#include <algorithm>

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
			auto c = target.get({ s.entity, s.variable });
			auto nc = c;
			auto frames = s.frames;

			std::sort(frames.begin(), frames.end());
			for (auto &kf : frames)
				nc.set(kf.t, kf.value);
		}
	}

	void GameRenderer::insertFrames(ExportedCurves import)
	{
		/*std::unique_lock<std::shared_mutex> ent_name_lock(EntNameMutex);

		for (auto &e : import.entity_names)
			EntityNames.insert({ e.second, e.first });

		ent_name_lock.unlock();

		auto &curves = getCurves();
		ImportSet(curves.boolCurves, import.boolCurves);
		ImportSet(curves.intCurves, import.intCurves);
		ImportSet(curves.intVectorCurves, import.intVectorCurves);
		ImportSet(curves.stringCurves, import.stringCurves);*/
	}

	void GameRenderer::installSystem(resources::system *system)
	{
		/*InstallSystem(system, SystemsMutex, Systems);*/
	}
}
