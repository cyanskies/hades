#ifndef HADES_LEVEL_HPP
#define HADES_LEVEL_HPP

#include "SFML/System/Time.hpp"

#include "hades/curve.hpp"
#include "Hades/GameInterface.hpp"
#include "Objects/Objects.hpp"

// a level is a playable area

namespace hades
{
	//load level from file
	/*  void load level()   */
	//load save from file
	/*  void load level save()   */

	constexpr auto level_ext = "lvl";
	constexpr auto save_ext = "hsv";

	using level_size_t = hades::types::int32;

	//A level is laid out in the same way as a save file
	// but doesn't store the full curve history
	struct level
	{
		hades::types::string name;
		hades::types::string description;

		//map size in pixels
		level_size_t map_x = 0, map_y = 0;
		//sequence of object
		//list of curve values, not including defaults
		std::vector<objects::object_info> objects;
		//the id of the next entity to be placed, or spawned in-game
		hades::EntityId next_id = hades::NO_ENTITY;

		//TODO: background, flat colour, paralax image, paralax loop image
		//set paralax to 0 to get a static image
		//layered images with different paralax?
		
		//TODO: tilemaps
		//TODO: terrain
	};

	struct level_save
	{
		//source level
		level source;

		//entity data
		curve_data curves;
		EntityId next_id = hades::NO_ENTITY;
		std::vector<std::tuple<types::string, EntityId>> names;
		
		//list of systems
		using system_list = std::vector<const resources::system*>;
		system_list systems;
		using system_attachment_list = std::vector<curve<sf::Time, std::vector<EntityId>>>;
		system_attachment_list systems_attached;

		//list of client systems?
		//nah, we get all that from the object base type,
		//they shouldn't be changing at runtime anyway
		// ^^ TODO: potential optimisation for the render instance?
	};

	level_save make_save_from_level(level l);
}

#endif //HADES_LEVEL_HPP
