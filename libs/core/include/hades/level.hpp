#ifndef HADES_LEVEL_HPP
#define HADES_LEVEL_HPP

#include "hades/curve.hpp"
#include "hades/level_interface.hpp"
#include "hades/objects.hpp"

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
		std::vector<object_instance> objects;
		//the id of the next entity to be placed, or spawned in-game
		entity_id next_id = bad_entity;

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
		entity_id next_id = bad_entity;
		name_curve_t names{ curve_type::step };
		
		//list of systems
		using system_list = std::vector<const resources::system*>;
		system_list systems;
		using system_attachment_list = std::vector<curve<resources::curve_types::vector_object_ref>>;
		system_attachment_list systems_attached;

		//list of client systems?
		//nah, we get all that from the object base type,
		//they shouldn't be changing at runtime anyway
		// ^^ TODO: potential optimisation for the render instance?
	};

	level_save make_save_from_level(level l);
}

#endif //HADES_LEVEL_HPP
