#ifndef HADES_SPRITE_BATCH_HPP
#define HADES_SPRITE_BATCH_HPP

#include <deque>

#include "SFML/Graphics/Drawable.hpp"
#include "SFML/Graphics/RenderStates.hpp"
#include "SFML/Graphics/Vertex.hpp"

#include "hades/exceptions.hpp"
#include "hades/rectangle_math.hpp"
#include "hades/strong_typedef.hpp"
#include "hades/time.hpp"
#include "hades/types.hpp"
#include "hades/vertex_buffer.hpp"

//sprite batching object

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
			// animations don't have to match, but texture, shader and uniforms must
			const resources::texture* texture;
			std::optional<resources::shader_proxy> shader_proxy; // stores uniforms and ref to shader
		};

		bool operator==(const sprite_settings &lhs, const sprite_settings &rhs);

		struct sprite_id_tag {};
		using sprite_id = strong_typedef<sprite_id_tag, int32>;
		constexpr auto bad_sprite_id = sprite_id{ std::numeric_limits<sprite_id::value_type>::min() };

		struct sprite
		{
			sprite_id id = bad_sprite_id;
			vector2_float position{};
			vector2_float size{};
			const resources::animation *animation = nullptr;
			time_point animation_progress;
			sprite_settings settings;
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
		using index_t = std::size_t;

		//clears all of the stored data
		void clear();
		void swap(sprite_batch&) noexcept;

		//NOTE: functions that accept sprite_id can throw sprite_batch_invalid_id
		sprite_id create_sprite();
		sprite_id create_sprite(const resources::animation *a, time_point t,
			sprite_utility::layer_t l, vector2_float p, vector2_float s, const resources::shader_uniform_map *u = {});
		bool exists(sprite_id id) const noexcept;
		void destroy_sprite(sprite_id id);

		void set_sprite(sprite_id, const resources::animation* a, time_point t,
			sprite_utility::layer_t l, vector2_float p, vector2_float s, const resources::shader_uniform_map *u = {});
		void set_sprite(sprite_id, time_point t, vector2_float p, vector2_float s);
		void set_position(sprite_id id, vector2_float pos);
		void set_animation(sprite_id id, const resources::animation *a, time_point t);
		void set_animation(sprite_id id, time_point t);
		void set_position_animation(sprite_id, vector2_float pos, const resources::animation* a, time_point t);
		void set_layer(sprite_id id, sprite_utility::layer_t l);
		void set_size(sprite_id id, vector2_float size);
		
		//NOTE: the position of the layer_t in the vector
		// indicates it's layer_index for draw
		std::vector<sprite_utility::layer_t> get_layer_list() const;
		
		struct layer_info {
			sprite_utility::layer_t l;
			index_t i;
		};

		// TODO: redesign rendering to avoid this function and its mandatory allocations (see render_interface)
		//returns a sorted vector of layer info,
		//pass the index type to draw to avoid a lookup
		std::vector<layer_info> get_layer_info_list() const;

		void apply();

		void draw(sf::RenderTarget& target, const sf::RenderStates& states = sf::RenderStates{}) const override;
		
		static_assert(!std::is_same_v<sprite_utility::layer_t, index_t>,
			"if these are the same then we cannot use index_t in api due to ambiguity");

		void draw(sf::RenderTarget& target, index_t layer_index, const sf::RenderStates& states = sf::RenderStates{}) const;

		// stores the sprites id and it's position in both the sprites info and quad batch arrays
		struct sprite_pos
		{
			sprite_id id;
			index_t index; // index into _vertex and _sprites
		};

		struct vert_batch
		{
			quad_buffer buffer;
			//this also holds the reference for the sprites pos in _sprites
			std::vector<sprite_id> sprites;
		};

	private:
		void _apply_changes();
		void _remove_empty_batch() noexcept; // remove the first empty batch
		void _add_sprite(sprite_utility::sprite);
		// removes sprite from its current batch and then places it back in the correct batch
		void _reseat_sprite(sprite_id, sprite_pos&, index_t);
		index_t _find_sprite(sprite_id) const;
		sprite_utility::sprite& _get_sprite(sprite_id);
		sprite_utility::sprite _remove_sprite(sprite_id, index_t current_batch, index_t buffer_index);
		
		std::vector<sprite_utility::batch> _sprites; // stores sprite information(pos, size, anim time, etc.)
		std::vector<vert_batch> _vertex; // stores sprite quad batches
		std::vector<sprite_pos> _ids;
		sprite_id _id_count = sprite_id{ static_cast<sprite_id::value_type>(sprite_utility::bad_sprite_id) + 1 };
	};

	inline void swap(sprite_batch& l, sprite_batch& r) noexcept
	{
		l.swap(r);
		return;
	}
}

#endif //HADES_SPRITE_BATCH_HPP
