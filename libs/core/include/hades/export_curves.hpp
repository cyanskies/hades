#ifndef HADES_EXPORTCURVES_HPP
#define HADES_EXPORTCURVES_HPP

#include "hades/curve_extra.hpp"
#include "hades/level_interface.hpp"

//Exported curves can be streamed into packets,
//they can also be imported into GameRenderers
//and into game instances in order to load from file
namespace hades
{
	//exported curves, need their type
	// curve type
	// entity and variable id's
	// 
	struct exported_curves
	{
		template<class T>
		struct export_set
		{
			entity_id entity = bad_entity;
			variable_id variable = variable_id::zero;
			//the client will use the variableid to ditermine curveType
		};

		std::vector<std::pair<entity_id, types::string>> entity_names;

		//TODO: this should be synced on connection only
		// this is covered by the unique_id sync table
		//std::vector<std::pair<VariableId, types::string>> variable_names;
		std::array<std::size_t, 11u> sizes;

		std::vector<export_set<resources::curve_types::int_t>> int_curves;
		std::vector<export_set<resources::curve_types::float_t>> float_curves;
		std::vector<export_set<resources::curve_types::vec2_float>> vec2_float_curves;
		std::vector<export_set<resources::curve_types::bool_t>> bool_curves;
		std::vector<export_set<resources::curve_types::string>> string_curves;
		std::vector<export_set<resources::curve_types::object_ref>> object_ref_curves;
		std::vector<export_set<resources::curve_types::unique>> unique_curves;
		//collection curves
		std::vector<export_set<resources::curve_types::collection_int>> int_vector_curves;
		std::vector<export_set<resources::curve_types::collection_float>> float_vector_curves;
		std::vector<export_set<resources::curve_types::collection_object_ref>> object_ref_vector_curves;
		std::vector<export_set<resources::curve_types::collection_unique>> unique_vector_curves;
	};
}

#endif //HADES_EXPORTCURVES_HPP