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
	//systems
	//scripts
	//holds a sfml texture objects
	using Texture = resources::texture;
	//holds a text string, used for dialogs and ui elements, can be overridden in mods to provide localisation
	using String = resources::string;
	//systems provide game logic on the server, or push sprites in the client.
	using System = resources::system;

	using CurveVariable = resources::curve;

	class DataManager : public data::data_manager
	{
	public:
		DataManager();

		Texture* getTexture(data::UniqueId);
		String* getString(data::UniqueId);
		System* getSystem(data::UniqueId);
		CurveVariable* getCurve(data::UniqueId);
	};

	extern DataManager* data_manager;
}

#endif // hades_data_hpp
