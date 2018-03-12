#ifndef HADES_SPRITE_BATCH_HPP
#define HADES_SPRITE_BATCH_HPP

#include <mutex>
#include <shared_mutex>

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/Vertex.hpp"
#include "SFML/System/Time.hpp"

#include "Hades/LimitedDraw.hpp"
#include "Hades/Types.hpp"

//A thread safe collection of animated sprites
//the object manages batching draw calls to render more efficiently
//the object also manages animation times.

namespace hades 
{
	namespace resources
	{
		struct animation;	
		struct shader;
		struct texture;
	}

	namespace sprite_utility
	{
		using layer_t = types::int32;

		struct SpriteSettings
		{
			layer_t layer = 0;
			const resources::texture* texture = nullptr;
			const resources::shader *shader = nullptr;
		};

		bool operator==(const SpriteSettings &lhs, const SpriteSettings &rhs);

		struct Sprite
		{
			Sprite() = default;
			Sprite(const Sprite &other);
			Sprite &operator=(const Sprite &other);

			using sprite_id = types::int64;

			std::mutex mut;
			sprite_id id;
			sf::Vector2f position;
			sf::Vector2f size;
			const resources::animation *animation = nullptr;
			sf::Time animation_progress;
		};

		bool operator==(const Sprite &lhs, const Sprite &rhs);

		using index_type = std::size_t;
		//				           //batch index,//sprite index, sprite ptr
		using found_sprite = std::tuple<index_type, index_type, Sprite*>;
		using batch = std::pair<sprite_utility::SpriteSettings, std::vector<sprite_utility::Sprite>>;
	}

	class SpriteBatch : public sf::Drawable, public Camera2dDrawClamp
	{
	public:
		using sprite_id = sprite_utility::Sprite::sprite_id;
		//clears all of the stored data
		void clear();

		sprite_id createSprite();
		sprite_id createSprite(const resources::animation *a, sf::Time t,
			sprite_utility::layer_t l, sf::Vector2f p, sf::Vector2f s);
		bool exists(sprite_id id) const;
		void destroySprite(sprite_id id);

		void setAnimation(sprite_id id, const resources::animation *a, sf::Time t);
		void setLayer(sprite_id id, sprite_utility::layer_t l);
		void setPosition(sprite_id id, sf::Vector2f pos);
		void setSize(sprite_id id, sf::Vector2f size);
		
		void limitDrawTo(const sf::FloatRect &worldCoords);

		void prepare();
		virtual void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates()) const override;

	private:
		//mutex to ensure two threads don't try to add/search/erase from the two collections at the same time
		mutable std::shared_mutex _collectionMutex; 

		using batch = sprite_utility::batch;
		std::vector<batch> _sprites;

		sf::FloatRect _drawArea;

		using vertex_batch = std::pair<sprite_utility::SpriteSettings, std::vector<sf::Vertex>>;
		std::vector<vertex_batch> _vertex;
		
		//to speed up usage of exists() cache all the id's from this frame
		std::vector<sprite_id> _used_ids;
		sprite_id _id_count = std::numeric_limits<sprite_id>::min();
	};
}

#endif //HADES_SPRITE_BATCH_HPP
