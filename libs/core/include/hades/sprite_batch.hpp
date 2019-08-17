#ifndef HADES_SPRITE_BATCH_HPP
#define HADES_SPRITE_BATCH_HPP

#include <deque>
#include <mutex>
#include <shared_mutex>

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/Vertex.hpp"

#include "hades/exceptions.hpp"
#include "hades/rectangle_math.hpp"
#include "hades/spinlock.hpp"
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
			using mutex_type = spinlock;

			sprite() noexcept = default;
			sprite(const sprite &other) noexcept ;
			sprite &operator=(const sprite &other) noexcept;

			sprite_id id = bad_sprite_id;
			vector_float position{};
			vector_float size{};
			layer_t layer{};
			const resources::animation *animation = nullptr;
			time_point animation_progress;
			mutable mutex_type mut;
		};

		bool operator==(const sprite &lhs, const sprite &rhs) noexcept;

		//TODO: move this to cpp if not needed in headercpp default noexcept movec
		using index_type = std::size_t;
		//				           //batch index,//sprite index, sprite ptr
		using found_sprite = std::tuple<index_type, index_type, sprite*>;

		struct batch
		{
			batch() noexcept = default;
			batch(sprite_settings s, std::vector<sprite> v) : settings{s}, sprites{std::move(v)}
			{}
			batch(const batch& rhs) : settings{rhs.settings}, sprites{rhs.sprites}
			{}
			batch& operator=(const batch& b)
			{
				settings = b.settings;
				sprites = b.sprites;
				return *this;
			}
			batch(batch&& rhs) : settings{ std::move(rhs.settings) }, sprites{ std::move(rhs.sprites) }
			{}
			batch& operator=(batch&& b)
			{
				settings = std::move(b.settings);
				sprites = std::move(b.sprites);
				return *this;
			}

			sprite_utility::sprite_settings settings;
			std::vector<sprite_utility::sprite> sprites;
			mutable shared_spinlock mutex;
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

		sprite_batch() = default;
		sprite_batch(const sprite_batch& other);
		sprite_batch(sprite_batch&& other) noexcept;

		sprite_batch& operator=(const sprite_batch&);
		sprite_batch& operator=(sprite_batch&&) noexcept;

		//clears all of the stored data
		void clear();
		void set_async(bool = true);
		void swap(sprite_batch&) noexcept;

		//===Thread Safe===
		//NOTE: functions that accept sprite_id can throw sprite_batch_invalid_id
		sprite_id create_sprite();
		sprite_id create_sprite(const resources::animation *a, time_point t,
			sprite_utility::layer_t l, vector_float p, vector_float s);
		bool exists(sprite_id id) const noexcept;
		void destroy_sprite(sprite_id id);

		void set_position(sprite_id id, vector_float pos);
		void set_animation(sprite_id id, const resources::animation *a, time_point t);
		void set_position_animation(sprite_id, vector_float pos, const resources::animation* a, time_point t);
		void set_layer(sprite_id id, sprite_utility::layer_t l);
		void set_size(sprite_id id, vector_float size);
		//===End Thread Safe===
		
		//NOTE: the position of the layer_t in the vector
		// indicates it's layer_index for draw(3)
		// this can be used to avoid the search needed to find the layer
		// in draw(2)
		std::vector<sprite_utility::layer_t> get_layer_list() const;

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
			vert_batch() = default;
			vert_batch(vert_batch&& v) noexcept : buffer{std::move(v.buffer)},
				sprites{std::move(v.sprites)}
			{}
			vert_batch& operator=(vert_batch&& v) noexcept
			{
				buffer = std::move(v.buffer);
				sprites = std::move(v.sprites);
				return *this;
			}
			vert_batch(const vert_batch& v) : buffer{ v.buffer },
				sprites{ v.sprites }
			{}
			vert_batch& operator=(const vert_batch& v)
			{
				buffer = v.buffer;
				sprites = v.sprites;
				return *this;
			}

			quad_buffer buffer;
			//this also holds the reference for the sprites pos in _sprites
			std::vector<sprite_id> sprites;
			mutable shared_spinlock mutex;
		};

		template<typename Func>
		void _apply_changes(sprite_id, Func f);
		void _add_sprite(sprite_utility::sprite);
		index_t _find_sprite(sprite_id) const;
		sprite_utility::sprite _remove_sprite(sprite_id, index_t current_batch, index_t buffer_index);
		
		mutable shared_mutex_type _sprites_mutex;
		std::deque<sprite_utility::batch> _sprites;

		mutable shared_mutex_type _vertex_mutex;
		std::deque<vert_batch> _vertex;
		
		mutable shared_mutex_type _id_mutex;
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
		index_t s_index;
		sprite s;
		{
			auto lock1 = std::shared_lock{ _sprites_mutex, std::defer_lock };
			auto lock2 = std::shared_lock{ _vertex_mutex, std::defer_lock };
			const auto lock = std::scoped_lock{ lock1, lock2 };

			auto& s_batch = _sprites[index];
			auto& v_batch = _vertex[index];

			auto s_b_read = std::shared_lock{ s_batch.mutex, std::defer_lock };
			auto v_b_read = std::shared_lock{ v_batch.mutex, std::defer_lock }; 
			std::lock(s_b_read, v_b_read);

			s_index = std::size(v_batch.sprites);
			for (auto i = index_t{}; i < std::size(v_batch.sprites); ++i)
			{
				if (v_batch.sprites[i] == id)
				{
					s_index = i;
					break;
				}
			}
			assert(s_index != std::size(v_batch.sprites));


			const auto sprite_lock = std::scoped_lock{ s_batch.sprites[s_index].mut };
			s = s_batch.sprites[s_index];
			assert(s.id == id);

			s = std::invoke(f, s);
			s_batch.sprites[s_index] = s;

			const auto new_settings = sprite_settings{ s.layer, s.animation->tex };
			if (new_settings == s_batch.settings)
			{
				//update v batch
				v_b_read.unlock();
				const auto lock = std::scoped_lock{ v_batch.mutex };
				const auto frame = animation::get_frame(*s.animation, s.animation_progress);
				_vertex[index].buffer.replace(make_quad_animation(s.position, s.size, *s.animation, frame), s_index);
				return;
			}
		}

		//if we reach here, then we need to move to a new batch
		_remove_sprite(id, index, s_index);
		_add_sprite(s);
		
		return;
	}
}

#endif //HADES_SPRITE_BATCH_HPP
