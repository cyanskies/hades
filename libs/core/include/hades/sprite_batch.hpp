#ifndef HADES_SPRITE_BATCH_HPP
#define HADES_SPRITE_BATCH_HPP

#include <mutex>
#include <shared_mutex>

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/Vertex.hpp"

#include "hades/rectangle_math.hpp"
#include "Hades/strong_typedef.hpp"
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

		struct sprite_id_tag {};
		using sprite_id = strong_typedef<sprite_id_tag, int32>;
		constexpr auto bad_sprite_id = sprite_id{ std::numeric_limits<sprite_id::value_type>::min() };

		struct sprite
		{
			sprite() = default;
			sprite(const sprite &other);
			sprite &operator=(const sprite &other);
			
			std::mutex mut;
			sprite_id id = bad_sprite_id;
			vector_float position{};
			vector_float size{};
			const resources::animation *animation = nullptr;
			time_point animation_progress;
		};

		bool operator==(const sprite &lhs, const sprite &rhs);

		//TODO: move this to cpp if not needed in headercpp default noexcept movec
		using index_type = std::size_t;
		//				           //batch index,//sprite index, sprite ptr
		using found_sprite = std::tuple<index_type, index_type, sprite*>;

		//TODO: named structs for this and the vertex batch
		using batch = std::pair<sprite_utility::sprite_settings, std::vector<sprite_utility::sprite>>;
	}

	class sprite_batch : public sf::Drawable
	{
	public:
		using sprite_id = sprite_utility::sprite_id;

		sprite_batch() = default;
		sprite_batch(const sprite_batch&);

		sprite_batch &operator=(const sprite_batch&);

		void swap(sprite_batch&);

		//clears all of the stored data
		void clear();

		//===Thread Safe===
		sprite_id create_sprite();
		sprite_id create_sprite(const resources::animation *a, time_point t,
			sprite_utility::layer_t l, vector_float p, vector_float s);
		bool exists(sprite_id id) const;
		void destroy_sprite(sprite_id id);

		void set_animation(sprite_id id, const resources::animation *a, time_point t);
		void set_layer(sprite_id id, sprite_utility::layer_t l);
		void set_position(sprite_id id, vector_float pos);
		void set_size(sprite_id id, vector_float size);
		//===End Thread Safe===

		void draw_clamp(const rect_float &worldCoords);

		void prepare();
		virtual void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates{}) const override;

	private:
		//mutex to ensure two threads don't try to add/search/erase from the two collections at the same time
		mutable std::shared_mutex _collection_mutex;  //TODO: split into sprite_mutex, vertex_mutex, id_mutex

		using batch = sprite_utility::batch;
		std::vector<batch> _sprites;

		rect_float _draw_clamp{};

		//TODO: vertex_buffer
		using vertex_batch = std::pair<sprite_utility::sprite_settings, std::vector<sf::Vertex>>;
		std::vector<vertex_batch> _vertex;
		
		//TODO: stack of returned ids, from destroy_sprite
		//to speed up usage of exists()
		std::vector<sprite_id> _used_ids;
		sprite_id _id_count = sprite_id{ static_cast<sprite_id::value_type>(sprite_utility::bad_sprite_id) + 1 };
	};

	void swap(sprite_batch&, sprite_batch&);
}

#endif //HADES_SPRITE_BATCH_HPP
