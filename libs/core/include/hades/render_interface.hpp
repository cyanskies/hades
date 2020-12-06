#ifndef HADES_RENDER_INTERFACE_HPP
#define HADES_RENDER_INTERFACE_HPP

#include <any>
#include <vector>

#include "SFML/Graphics/Drawable.hpp"

#include "hades/exceptions.hpp"
#include "hades/level_interface.hpp"
#include "hades/sprite_batch.hpp"

namespace hades
{
	class render_interface_error : public runtime_error
	{
	public:
		using runtime_error::runtime_error;
	};

	class render_interface_invalid_id : public render_interface_error
	{
	public:
		using render_interface_error::render_interface_error;
	};

	//allows batching of sprites and drawables
	// drawables on the same layer as sprites
	// will be draw before the sprites

	class render_interface final : public sf::Drawable
	{
	public:
		using sprite_id = sprite_utility::sprite_id;
		using sprite_layer = sprite_utility::layer_t;

		struct drawable_id_tag {};
		using drawable_id = strong_typedef<drawable_id_tag, int32>;
		using drawable_layer = sprite_layer;

		static constexpr drawable_id bad_drawable_id = drawable_id{ std::numeric_limits<drawable_id::value_type>::min() };
		using get_drawable = const sf::Drawable& (*)(const std::any&);

		struct drawable_object
		{
			drawable_id id = bad_drawable_id;
			std::any storage;
			get_drawable get = nullptr;
		};

		struct object_layer
		{
			sprite_layer layer;
			std::vector<drawable_object> objects;
		};

		static_assert(std::is_nothrow_move_assignable_v<object_layer>);

		sprite_id create_sprite();
		sprite_id create_sprite(const resources::animation *, time_point,
			sprite_layer, vector_float position, vector_float size);

		bool sprite_exists(sprite_id) const noexcept;
		//NOTE: functions that accept sprite_id or drawable_id can throw render_instance_invalid_id
		void destroy_sprite(sprite_id);

		void set_sprite(sprite_id, const resources::animation*, time_point,
			sprite_layer, vector_float position, vector_float size);
		void set_sprite(sprite_id, time_point, vector_float position, vector_float size);
		void set_animation(sprite_id, const resources::animation *, time_point);
		void set_animation(sprite_id, time_point);
		void set_layer(sprite_id, sprite_layer);
		void set_position(sprite_id, vector_float position);
		void set_size(sprite_id, vector_float size);

		drawable_id create_drawable();

		drawable_id create_drawable_ptr(const sf::Drawable*, sprite_layer);
		template<typename DrawableObject>
		drawable_id create_drawable_copy(DrawableObject&&, sprite_layer);

		bool drawable_exists(drawable_id) const noexcept;

		void update_drawable_ptr(drawable_id, const sf::Drawable*, sprite_layer);
		template<typename DrawableObject>
		void update_drawable_copy(drawable_id, DrawableObject&&, sprite_layer);

		void destroy_drawable(drawable_id);

		void prepare(); //must be called before draw if using sprites
		//NOTE: sprite_batch uses quad_buffer internally, for consistancy,
		//		transforms passed through sf::RenderStates won't be applied
		//		to anything drawn with the interface
		void draw(sf::RenderTarget&, sf::RenderStates = sf::RenderStates{}) const;

	private:
		drawable_id _make_new_id();
		void _destroy_id(drawable_id);
		void _add_to_layer(drawable_object, sprite_layer);
		void _remove_from_layer(std::vector<object_layer>::size_type layer_index,
			std::vector<drawable_object>::size_type obj_index) noexcept;
		drawable_id _create_drawable_any(std::any drawable, get_drawable, sprite_layer);
		void _update_drawable_any(drawable_id, std::any drawable, get_drawable, sprite_layer);

		sprite_batch _sprite_batch;

		std::vector<drawable_id> _used_drawable_ids;
		drawable_id _drawable_id = bad_drawable_id;
		std::vector<object_layer> _object_layers;
	};

	namespace detail
	{
		template<typename T>
		inline const sf::Drawable& get_drawable_from_any(const std::any& a) noexcept
		{
			return std::any_cast<&T>(a);
		}
	}

	template<typename DrawableObject>
	inline render_interface::drawable_id render_interface::create_drawable_copy(DrawableObject &&d, sprite_layer l)
	{
		return _create_drawable_any(std::forward(d), detail::get_drawable_from_any<Drawable>, l);
	}

	template<typename DrawableObject>
	inline void render_interface::update_drawable_copy(drawable_id id, DrawableObject &&d, sprite_layer l)
	{
		_update_drawable_any(id, std::forward(d),
			detail::get_drawable_from_any<Drawable>, l);
	}
}

#endif //!HADES_RENDER_INTERFACE_HPP
