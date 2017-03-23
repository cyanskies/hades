#ifndef HADES_GAMEINSTANCE_HPP
#define HADES_GAMEINSTANCE_HPP

#include "SFML/System/Time.hpp"

namespace hades
{
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
		//std::map<entityname_pair, curve*> curves

		//entity id next

		//systems(systems run logic by added keyframes to curves(and adding frame predictions)
		//how to handle events?(events are stored as pulse curves, the system will have to check the curve to see if theirs an event it is interested in
	};
}

#endif //HADES_GAMEINSTANCE_HPP
