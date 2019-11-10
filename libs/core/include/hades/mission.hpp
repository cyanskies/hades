#ifndef HADES_MISSION_HPP
#define HADES_MISSION_HPP

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
		//seperate levels
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
		//std::vector<unique_id, level_save>
	};

	string serialise(const mission&);
	mission deserialise_mission(std::string_view);

	mission_save make_save_from_mission(mission l);
}

#endif //HADES_MISSION_HPP
