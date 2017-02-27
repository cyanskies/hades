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
	}

	Texture* DataManager::getTexture(data::UniqueId key)
	{
		return get<Texture>(key);
	}

	String* DataManager::getString(data::UniqueId key)
	{
		return get<String>(key);
	}
}