#ifndef HADES_LEVEL_HPP
#define HADES_LEVEL_HPP

#include "hades/level_interface.hpp"
#include "hades/math.hpp"
#include "hades/objects.hpp"
#include "hades/colour.hpp"
#include "hades/curve.hpp"
#include "hades/uniqueid.hpp"

// a level is a playable area

namespace hades
{
	//load level from file
	/*  void load level()   */
	//load save from file
	/*  void load level save()   */

	constexpr auto level_ext = "lvl";
	constexpr auto save_ext = "sav";

	using level_size_t = hades::types::int32;

	//A level is laid out in the same way as a save file
	// but doesn't store the full curve history
	struct level
	{
		struct background_layer
		{
			//paralax controls the movement speed of the background relative to
			//the world; 0.f is a static image, 1.f is no paralax
			vector_float offset{};
			vector_float parallax = { 1.f, 1.f };
			//if animation is smaller than the world then it will loop
			unique_id animation = unique_id::zero;
		};

		struct region
		{
			colour colour;
			rect_float bounds;
			string name;
		};

		hades::types::string name;
		hades::types::string description;

		//map size in pixels
		level_size_t map_x = 0, map_y = 0;
		//sequence of object
		//list of curve values, not including defaults
		std::vector<object_instance> objects;
		//the id of the next entity to be placed, or spawned in-game
		entity_id next_id = entity_id{ static_cast<entity_id::value_type>(bad_entity) + 1 };

		//backgrounds
		colour background_colour = colours::black;
		std::vector<background_layer> background_layers;
		
		//trigger regions
		std::vector<region> regions;

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

	string serialise(const level&);
	level deserialise(std::string_view);

	level_save make_save_from_level(level l);
}

#endif //HADES_LEVEL_HPP
