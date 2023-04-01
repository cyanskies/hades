#ifndef HADES_MISSION_HPP
#define HADES_MISSION_HPP

#include "hades/level.hpp"
#include "hades/objects.hpp"
#include "hades/time.hpp"

namespace hades
{
	constexpr auto mission_ext = "mission";
	constexpr auto save_ext = "msav";

	struct mission
	{
		string name;
		string description;

		//players
		struct player
		{
			unique_id id = unique_zero;
			entity_id object = bad_entity;
		};

		std::vector<player> players;

		//game objects
		object_data objects;

		//scripts
		//on_load
		//on_save
		//on_tick

		//inline levels
		struct level_element
		{
			unique_id name = unique_id::zero;
            level level_obj{};
		};
		std::vector<level_element> inline_levels;

		//seperate levels
		struct external_level 
		{
			unique_id name = unique_id::zero;
			string path;
		};
		std::vector<external_level> external_levels;
	};

	struct mission_save
	{
		//source level
		mission source;

		//entity data
		object_save_data objects;

		//level saves
		struct level_save_element
		{
			unique_id name = unique_id::zero;
			level_save save;
		};
		std::vector<level_save_element> level_saves;

		time_point time;
	};

	// TODO: straight to writer
	string serialise(const mission&);
	// TODO: straight from parser
	mission deserialise_mission(std::string_view);

	mission_save make_save_from_mission(mission l);
}

#endif //HADES_MISSION_HPP
