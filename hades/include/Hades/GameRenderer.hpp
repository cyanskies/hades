#ifndef HADES_GAMERENDERER_HPP
#define HADES_GAMERENDERER_HPP

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/Sprite.hpp"

#include "Hades/ExportedCurves.hpp"
#include "Hades/GameInterface.hpp"
#include "Hades/shared_map.hpp"
#include "Hades/systems.hpp"

//TODO: merge into systems.hpp

namespace hades
{
	using RenderSystem = GameSystem;

	class GameRenderer;

	struct render_system_data
	{
		//entities that should be worked on by this system
		std::vector<EntityId> entities;
		//game_data to get variables from and to create/modify sprites
		GameRenderer* game_data;
		//current game time
		sf::Time current_time;
	};

	class RendererInterface : public GameInterface
	{
	public:
		//controls for messing with sprites
		using SpriteMap = shared_map<EntityId, sf::Sprite>;
		SpriteMap &getSprites();
	protected:
		SpriteMap _sprites;
	};

	class GameRenderer : public sf::Drawable, public RendererInterface
	{
	public:
		virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
		
		//sets all the sprites positions and animation states
		//for the current time
		void prepareFrame();

		//sets the current game clock
		void setTime(sf::Time time);
		//progresses the clock by dt
		void addTime(sf::Time dt);

		//insert curvestream
		void insertFrames(ExportedCurves import);

		//systems
		void installSystem(resources::system *system);
	private:
		//the time that should be drawn/generated
		sf::Time _currentTime;
	};
}

#endif //HADES_GAMERENDERER_HPP
