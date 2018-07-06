#ifndef HADES_EXPORTEDCURVES_HPP
#define HADES_EXPORTEDCURVES_HPP

#include "SFML/System/Time.hpp"

#include "hades/curve.hpp"
#include "Hades/GameInterface.hpp"

//Exported curves can be streamed into packets,
//they can also be imported into GameRenderers
//and into game instances in order to load from file
namespace hades
{
	//exported curves, need their type
	// curve type
	// entity and variable id's
	// 
	struct ExportedCurves
	{
		template<class T>
		struct ExportSet
		{
			EntityId entity;
			VariableId variable;
			//the client will use the variableid to ditermine curveType
			std::vector<keyframe<sf::Time, T>> frames;
		};

		std::vector<std::pair<EntityId, types::string>> entity_names;

		//TODO: this should be synced on connection only
		//std::vector<std::pair<VariableId, types::string>> variable_names;

		//each curve type goes here
		std::vector < ExportSet<types::int32>> intCurves;
		std::vector < ExportSet<float>> floatCurves;
		std::vector < ExportSet<bool>> boolCurves;
		std::vector < ExportSet<types::string>> stringCurves;
		std::vector < ExportSet<unique_id>> uniqueCurves;

		std::vector < ExportSet<std::vector<types::int32>>> intVectorCurves;
		std::vector < ExportSet<std::vector<float>>> floatVectorCurves;
		std::vector < ExportSet<std::vector<unique_id>>> uniqueVectorCurves;
	};
}

#endif //HADES_EXPORTEDCURVES_HPP