#ifndef HADES_GAMERENDERER_HPP
#define HADES_GAMERENDERER_HPP

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/Sprite.hpp"

#include "Hades/GameInterface.hpp"
#include "Hades/GameSystem.hpp"
#include "Hades/transactional_map.hpp"

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
		//controlls for messing with sprites
		using SpriteMap = transactional_map<EntityId, sf::Sprite>;
		SpriteMap &getSprites();
	private:
		//the time that should be drawn/generated
		sf::Time _currentTime;

		using SpriteMap = transactional_map<EntityId, sf::Sprite>;
		SpriteMap _sprites;
	};

	class GameRenderer : public sf::Drawable
	{
		//sets the current game clock
		void setTime(sf::Time time);
		//progresses the clock by dt
		void addTime(sf::Time dt);

		//insert curvestream
		void insertFrames();
	};
}

#endif //HADES_GAMERENDERER_HPP