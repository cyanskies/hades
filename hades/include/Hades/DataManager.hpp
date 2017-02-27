#ifndef HADES_DATA_HPP
#define HADES_DATA_HPP

#include "Hades/data_manager.hpp"
#include "Hades/simple_resources.hpp"


namespace hades
{
	//default hades datamanager
	//registers that following resource types:
	//texture
	//sound
	//document
	//string store
	using Texture = resources::texture;
	using String = resources::string;

	class DataManager : public data::data_manager
	{
	public:
		DataManager();

		Texture* getTexture(data::UniqueId);
		String* getString(data::UniqueId);
	};

	extern DataManager* data_manager;
}

#endif // hades_data_hpp
