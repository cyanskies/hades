#ifndef HADES_RENDER_INTERFACE_HPP
#define HADES_RENDER_INTERFACE_HPP

#include "hades/level_interface.hpp"
#include "hades/sprite_batch.hpp"

namespace hades
{
	class render_interface
	{
	public:
		using sprite_id = sprite_utility::sprite_id;
		using sprite_layer = sprite_utility::layer_t;

		//===Thread-Safe===

		sprite_id create_sprite();
		sprite_id create_sprite(const resources::animation *, time_point,
			sprite_layer, vector_float position, vector_float size);
		bool sprite_exists(sprite_id) const;
		void destroy_sprite(sprite_id);

		void set_animation(sprite_id, const resources::animation *, time_point);
		void set_layer(sprite_id, sprite_layer);
		void set_position(sprite_id, vector_float position);
		void set_size(sprite_id, vector_float size);

		struct drawable_id_tag {};
		using drawable_id = strong_typedef<drawable_id_tag, int32>;
		using drawable_layer = sprite_layer;

		static constexpr drawable_id bad_drawable_id = drawable_id{ std::numeric_limits<drawable_id::value_type>::min() };

		using get_drawable = sf::Drawable& (*)(std::any&);

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

		drawable_id create_drawable();

		drawable_id create_drawable_ptr(const sf::Drawable*, sprite_layer);
		template<typename Drawable>
		drawable_id create_drawable_copy(Drawable, sprite_layer);

		void update_drawable_ptr(const sf::Drawable*, sprite_layer);
		template<typename Drawable>
		void update_drawable_copy(Drawable, sprite_layer);

		void destroy_drawable(drawable_id);

		//===End Thread-Safe===

	private:
		drawable_id _create_drawable_any(std::any drawable, get_drawable, sprite_layer);

		//sprite batch is already thread safe
		sprite_batch _sprite_batch;
		mutable std::shared_mutex _object_mutex;
		std::vector<object_layer> _object_layers;
	};
}

#endif //HADES_RENDER_INTERFACE_HPP
