#ifndef HADES_LEVEL_HPP
#define HADES_LEVEL_HPP

// TODO: order these
#include "hades/math.hpp"
#include "hades/objects.hpp"
#include "hades/colour.hpp"
#include "hades/curve.hpp"
#include "hades/terrain.hpp"
#include "hades/tiles.hpp"
#include "hades/uniqueid.hpp"

// a level is a playable area

namespace hades
{
	//load level from file
	/*  void load level()   */
	//load save from file
	/*  void load level save()   */

	constexpr auto level_ext = "lvl";
	constexpr auto level_save_ext = "sav";

	// TODO: move to game types?
	using level_size_t = hades::types::int32;
	using rect_level = hades::rect_t<level_size_t>;

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
		
		//objects
		object_data objects;

		//backgrounds
		colour background_colour = colours::black;
		std::vector<background_layer> background_layers;
		
		//trigger regions
		std::vector<region> regions;

		//level scripts
		unique_id on_load = unique_zero;
		//on_save
		//on_tick

		//player and ai input script
		unique_id player_input_script = unique_id::zero,
			ai_input_script = unique_id::zero;

		//tile map
		raw_map tile_map_layer;
		
		//terrain map
		unique_id terrainset = unique_id::zero;
		std::vector<terrain_id_t> terrain_vertex;
		std::vector<raw_map> terrain_layers;
	};
	
	struct level_save
	{
		//source level
		level source;

		//total amount of unticked time when compared to the mission
		[[deprecated]] time_duration sleep_time{};
		time_point level_time; // the last tick time before being saved
		//entity data
		object_save_data objects;

		//using system_list = object_save::system_list;
		//using system_attachment_list = object_save::system_attachment_list;

		//TODO: input curve
	};

	void serialise(const level&, data::writer&);
	string serialise(const level&);
	level deserialise_level(std::string_view);
	level deserialise_level(data::parser_node&);

	level_save make_save_from_level(level l);
}

#endif //HADES_LEVEL_HPP
