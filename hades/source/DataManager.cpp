#include "Hades/DataManager.hpp"

#include "Hades/simple_resources.hpp"

namespace hades
{
	DataManager *data_manager = nullptr;

	DataManager::DataManager()
	{
		//register custom resource types	
		register_resource_type("actions", nullptr);
		register_resource_type("animations", resources::parseAnimation);
		register_resource_type("curves", resources::parseCurve);
		register_resource_type("fonts", resources::parseFont);
		register_resource_type("strings", resources::parseString);
		register_resource_type("systems", resources::parseSystem);
		register_resource_type("textures", resources::parseTexture);
	}

	Animation* DataManager::getAnimation(data::UniqueId key)
	{
		return get<Animation>(key);
	}

	CurveVariable* DataManager::getCurve(data::UniqueId key)
	{
		return get<CurveVariable>(key);
	}

	Font* DataManager::getFont(data::UniqueId key)
	{
		return get<Font>(key);
	}

	String* DataManager::getString(data::UniqueId key)
	{
		return get<String>(key);
	}

	System* DataManager::getSystem(data::UniqueId key)
	{
		return get<System>(key);
	}

	Texture* DataManager::getTexture(data::UniqueId key)
	{
		return get<Texture>(key);
	}
}
