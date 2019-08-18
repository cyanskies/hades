#ifndef HADES_SPRITE_BATCH_HPP
#define HADES_SPRITE_BATCH_HPP

#include <deque>

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/Vertex.hpp"

#include "hades/exceptions.hpp"
#include "hades/rectangle_math.hpp"
#include "hades/strong_typedef.hpp"
#include "hades/time.hpp"
#include "hades/types.hpp"
#include "hades/vertex_buffer.hpp"

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
			layer_t layer;
			const resources::texture* texture;
			//const resources::shader *shader;
		};

		bool operator==(const sprite_settings &lhs, const sprite_settings &rhs);

		struct sprite_id_tag {};
		using sprite_id = strong_typedef<sprite_id_tag, int32>;
		constexpr auto bad_sprite_id = sprite_id{ std::numeric_limits<sprite_id::value_type>::min() };

		struct sprite
		{
			sprite_id id = bad_sprite_id;
			vector_float position{};
			vector_float size{};
			layer_t layer{};
			const resources::animation *animation = nullptr;
			time_point animation_progress;
		};

		bool operator==(const sprite &lhs, const sprite &rhs) noexcept;

		//TODO: move this to cpp if not needed in headercpp default noexcept movec
		using index_type = std::size_t;
		//				           //batch index,//sprite index, sprite ptr
		using found_sprite = std::tuple<index_type, index_type, sprite*>;

		struct batch
		{
			sprite_utility::sprite_settings settings;
			std::vector<sprite_utility::sprite> sprites;
		};
	}

	class sprite_batch_error : public runtime_error
	{
	public:
		using runtime_error::runtime_error;
	};

	class sprite_batch_invalid_id : public sprite_batch_error
	{
	public:
		using sprite_batch_error::sprite_batch_error;
	};

	class sprite_batch final : public sf::Drawable
	{
	public:
		using sprite_id = sprite_utility::sprite_id;
		using shared_mutex_type = shared_spinlock;
		using mutex_type = spinlock;
		using index_t = std::size_t;

		//clears all of the stored data
		void clear();
		void swap(sprite_batch&) noexcept;

		//NOTE: functions that accept sprite_id can throw sprite_batch_invalid_id
		sprite_id create_sprite();
		sprite_id create_sprite(const resources::animation *a, time_point t,
			sprite_utility::layer_t l, vector_float p, vector_float s);
		bool exists(sprite_id id) const noexcept;
		void destroy_sprite(sprite_id id);

		void set_sprite(sprite_id, const resources::animation* a, time_point t,
			sprite_utility::layer_t l, vector_float p, vector_float s);
		void set_position(sprite_id id, vector_float pos);
		void set_animation(sprite_id id, const resources::animation *a, time_point t);
		void set_position_animation(sprite_id, vector_float pos, const resources::animation* a, time_point t);
		void set_layer(sprite_id id, sprite_utility::layer_t l);
		void set_size(sprite_id id, vector_float size);
		
		//NOTE: the position of the layer_t in the vector
		// indicates it's layer_index for draw(3)
		// this can be used to avoid the search needed to find the layer
		// in draw(2)
		std::vector<sprite_utility::layer_t> get_layer_list() const;
		
		struct layer_info {
			sprite_utility::layer_t l;
			index_t i;
		};
		//returns a sorted vector of layer info,
		//pass the index type to draw to avoid a lookup
		std::vector <layer_info> get_layer_info_list() const;

		void apply();

		void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates{}) const override;
		void draw(sf::RenderTarget& target, sprite_utility::layer_t, sf::RenderStates states = sf::RenderStates{}) const;

		static_assert(!std::is_same_v<sprite_utility::layer_t, index_t>,
			"if these are the same then we cannot use index_t in api due to ambiguity");

		void draw(sf::RenderTarget& target, index_t layer_index, sf::RenderStates states = sf::RenderStates{}) const;

	private:
		struct sprite_pos 
		{
			sprite_id id;
			index_t index;
		};

		struct vert_batch
		{
			quad_buffer buffer;
			//this also holds the reference for the sprites pos in _sprites
			std::vector<sprite_id> sprites;
		};

		template<typename Func>
		void _apply_changes(sprite_id, Func f);
		void _add_sprite(sprite_utility::sprite);
		index_t _find_sprite(sprite_id) const;
		sprite_utility::sprite _remove_sprite(sprite_id, index_t current_batch, index_t buffer_index);
		
		std::deque<sprite_utility::batch> _sprites;
		std::deque<vert_batch> _vertex;
		std::vector<sprite_pos> _ids;
		sprite_id _id_count = sprite_id{ static_cast<sprite_id::value_type>(sprite_utility::bad_sprite_id) + 1 };
	};

	inline void swap(sprite_batch& l, sprite_batch& r) noexcept
	{
		l.swap(r);
		return;
	}

	template<typename Func>
	void sprite_batch::_apply_changes(sprite_id id, Func f)
	{
		static_assert(std::is_nothrow_invocable_r_v<sprite, Func, sprite>,
			"Func must accept a sprite and return a sprite as noexcept");

		const auto index = _find_sprite(id);
	
		auto& s_batch = _sprites[index];
		auto& v_batch = _vertex[index];

		const auto s_index = [id, &v_batch]() {
			for (auto i = index_t{}; i < std::size(v_batch.sprites); ++i)
			{
				if (v_batch.sprites[i] == id)
					return i;
			}
			return std::size(v_batch.sprites);
		}();
		assert(s_index != std::size(v_batch.sprites));

		auto s = s_batch.sprites[s_index];
		assert(s.id == id);

		s = std::invoke(f, s);
		s_batch.sprites[s_index] = s;

		const auto new_settings = sprite_settings{ s.layer, s.animation->tex };
		if (new_settings == s_batch.settings)
		{
			const auto frame = animation::get_frame(*s.animation, s.animation_progress);
			_vertex[index].buffer.replace(make_quad_animation(s.position, s.size, *s.animation, frame), s_index);
			return;
		}
	
		//if we reach here, then we need to move to a new batch
		_remove_sprite(id, index, s_index);
		_add_sprite(s);
		
		return;
	}
}

#endif //HADES_SPRITE_BATCH_HPP
