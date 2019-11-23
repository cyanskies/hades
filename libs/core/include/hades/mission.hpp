#ifndef HADES_MISSION_HPP
#define HADES_MISSION_HPP

#include "hades/level.hpp"
#include "hades/level_curve_data.hpp"
#include "hades/objects.hpp"

namespace hades
{
	constexpr auto mission_ext = "mission";
	constexpr auto save_ext = "msav";

	struct mission
	{
		string name;
		string description;

		//players

		//sequence of object
		//list of curve values, not including defaults
		std::vector<object_instance> objects;
		//the id of the next entity to be placed, or spawned in-game
		entity_id next_id = next(bad_entity);

		//scripts
		//on_load
		//on_save
		//on_tick

		//inline levels
		struct level_element
		{
			unique_id name = unique_id::zero;
			level level{};
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
		curve_data curves;
		entity_id next_id = bad_entity;

		name_curve_t names{ curve_type::step, name_curve_t::value_type{} };

		//level saves
		struct level_save_element
		{
			unique_id name = unique_id::zero;
			level_save save;
		};
		std::vector<level_save_element> level_saves;
	};

	string serialise(const mission&);
	mission deserialise_mission(std::string_view);

	mission_save make_save_from_mission(mission l);
}

#endif //HADES_MISSION_HPP
