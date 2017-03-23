#ifndef HADES_GAMEINSTANCE_HPP
#define HADES_GAMEINSTANCE_HPP

#include <map>
#include <memory>
#include <vector>

#include "SFML/System/Time.hpp"

#include "Hades/Curves.hpp"
#include "Hades/Types.hpp"
#include "Hades/value_guard.hpp"

namespace hades
{
	using EntityId = int;
	using VariableId = int;
	using CurveId = std::pair<EntityId, VariableId>;

	//represents the logic side of an individual level or game
	//TODO:
	// support saving
	// support loading
	// 
	class GameInstance
	{
	public:
		void tick(sf::Time dt);

	private:
		//master entity(contains game settings and so on eg. timers, the terrain entity, references to important names entities for mission objectives)
		//entity list //converts names to ids, not all entities have names, but some must for scripting purposes

		//curve list
		//a curve has an entity id,that it's attached too, and a name
		template<class T>
		using GameCurve = Curve<sf::Time, T>;
		template<class T>
		using Curve_ptr = std::unique_ptr< value_guard< GameCurve<T> > >;
		template<class T>
		using CurveMap = std::map< CurveId, Curve_ptr<T> >;

		CurveMap<types::int32> _intCurves;
		//no linear curves here
		CurveMap<bool> _boolCurves;
		CurveMap<types::string> _stringCurves;

		//vector curves
		//no linear curves for these either
		CurveMap<std::vector<types::int32>> _intVectorCurves;
		CurveMap<std::vector<bool>> _boolVectorCurves;
		CurveMap<std::vector<types::string>> _stringVectorCurves;


		//entity id next

		//systems(systems run logic by added keyframes to curves(and adding frame predictions)
		//how to handle events?(events are stored as pulse curves, the system will have to check the curve to see if theirs an event it is interested in
	};
}

#endif //HADES_GAMEINSTANCE_HPP
