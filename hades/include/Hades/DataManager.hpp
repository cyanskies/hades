#ifndef HADES_DATAMANAGER_EX_HPP
#define HADES_DATAMANAGER_EX_HPP

#include "Hades/data_manager.hpp"
#include "Hades/simple_resources.hpp"

//TODO: remove this header

namespace hades
{
	//default hades datamanager
	//registers that following resource types:
	
	//holds a sfml texture objects
	using Texture = resources::texture;
	//holds a text string, used for dialogs and ui elements, can be overridden in mods to provide localisation
	using String = resources::string;
	//systems provide game logic on the server, or push sprites in the client.
	using System = resources::system;
	//curve variables define how a curve should act and what kinds of values it should hold
	using CurveVariable = resources::curve;
	//animations decribe visible sprites and changes that occur to them over time
	using Animation = resources::animation;
	//fonts can be drawn with the sf::text class
	using Font = resources::font;
	//actions define input names that are used by the gameinstance
	//using Action = ///;

	//TODO:
	//describes a playable sound effect
	//sound
	//decribes a music piece that can be played by the jukebox
	//document//?

	class DataManager : public data::data_manager
	{
	public:
		DataManager();

		Animation* getAnimation(data::UniqueId);
		CurveVariable* getCurve(data::UniqueId);
		Font* getFont(data::UniqueId);
		String* getString(data::UniqueId);
		System* getSystem(data::UniqueId);
		Texture* getTexture(data::UniqueId);
	};

	extern DataManager* data_manager;
}

#endif // !hades_datamanager_ex_hpp
