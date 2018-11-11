#ifndef HADES_SPRITE_BATCH_HPP
#define HADES_SPRITE_BATCH_HPP

#include <mutex>
#include <shared_mutex>

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/Vertex.hpp"

#include "hades/math.hpp"
#include "hades/timers.hpp"
#include "hades/types.hpp"

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

		struct sprite_settings
		{
			layer_t layer = 0;
			const resources::texture* texture = nullptr;
			const resources::shader *shader = nullptr;
		};

		bool operator==(const sprite_settings &lhs, const sprite_settings &rhs);

		struct sprite
		{
			sprite() = default;
			sprite(const sprite &other);
			sprite &operator=(const sprite &other);
			
			using sprite_id = int64;

			static constexpr auto bad_sprite_id = std::numeric_limits<sprite_id>::min();

			std::mutex mut;
			sprite_id id = bad_sprite_id;
			vector_float position{};
			vector_float size{};
			const resources::animation *animation = nullptr;
			time_point animation_progress;
		};

		bool operator==(const sprite &lhs, const sprite &rhs);

		using index_type = std::size_t;
		//				           //batch index,//sprite index, sprite ptr
		using found_sprite = std::tuple<index_type, index_type, sprite*>;
		using batch = std::pair<sprite_utility::sprite_settings, std::vector<sprite_utility::sprite>>;
	}

	class sprite_batch : public sf::Drawable
	{
	public:
		using sprite_id = sprite_utility::sprite::sprite_id;

		sprite_batch() = default;
		sprite_batch(const sprite_batch&);

		sprite_batch &operator=(const sprite_batch&);

		//clears all of the stored data
		void clear();

		sprite_id create_sprite();
		sprite_id create_sprite(const resources::animation *a, time_point t,
			sprite_utility::layer_t l, vector_float p, vector_float s);
		bool exists(sprite_id id) const;
		void destroy_sprite(sprite_id id);

		void set_animation(sprite_id id, const resources::animation *a, time_point t);
		void set_layer(sprite_id id, sprite_utility::layer_t l);
		void set_position(sprite_id id, vector_float pos);
		void set_size(sprite_id id, vector_float size);
		
		void draw_clamp(const rect_float &worldCoords);

		void prepare();
		virtual void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates{}) const override;

	private:
		//mutex to ensure two threads don't try to add/search/erase from the two collections at the same time
		mutable std::shared_mutex _collection_mutex; 

		using batch = sprite_utility::batch;
		std::vector<batch> _sprites;

		rect_float _draw_clamp{};

		using vertex_batch = std::pair<sprite_utility::sprite_settings, std::vector<sf::Vertex>>;
		std::vector<vertex_batch> _vertex;
		
		//to speed up usage of exists() cache all the id's from this frame
		std::vector<sprite_id> _used_ids;
		sprite_id _id_count = sprite_utility::sprite::bad_sprite_id + 1;
	};
}

#endif //HADES_SPRITE_BATCH_HPP
