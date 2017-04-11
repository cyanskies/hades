#include "Hades/DataManager.hpp"

#include "Hades/simple_resources.hpp"

namespace hades
{
	DataManager *data_manager = nullptr;

	DataManager::DataManager()
	{
		//register custom resource types
		register_resource_type("textures", resources::parseTexture);
		register_resource_type("strings", resources::parseString);
		register_resource_type("systems", resources::parseSystem);
		register_resource_type("curves", resources::parseCurve);
	}

	Texture* DataManager::getTexture(data::UniqueId key)
	{
		return get<Texture>(key);
	}

	String* DataManager::getString(data::UniqueId key)
	{
		return get<String>(key);
	}

	System* DataManager::getSystem(data::UniqueId key)
	{
		return get<System>(key);
	}

	CurveVariable* DataManager::getCurve(data::UniqueId key)
	{
		return get<CurveVariable>(key);
	}
}
