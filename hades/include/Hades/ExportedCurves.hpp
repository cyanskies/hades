#ifndef HADES_EXPORTEDCURVES_HPP
#define HADES_EXPORTEDCURVES_HPP

#include "SFML/System/Time.hpp"

#include "Hades/Curves.hpp"
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
			CurveType type;
			std::vector<Keyframe<sf::Time, T>> frames;
		};

		std::vector<std::pair<EntityId, types::string>> entity_names;
		std::vector<std::pair<VariableId, types::string>> variable_names;

		//each curve type goes here
		std::vector < ExportSet<types::int32>> intCurves;
		std::vector < ExportSet<bool>> boolCurves;
		std::vector < ExportSet<types::string>> stringCurves;
		std::vector < ExportSet<std::vector<types::int32>>> intVectorCurves;
	};
}

#endif //HADES_EXPORTEDCURVES_HPP