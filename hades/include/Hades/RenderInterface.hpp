#ifndef HADES_RENDERINTERFACE_HPP
#define HADES_RENDERINTERFACE_HPP

#include "hades/level_interface.hpp"
#include "hades/sprite_batch.hpp"

namespace hades
{
	class RendererInterface : public game_interface
	{
	public:
		//controls for messing with sprites
		sprite_batch  *getSprites();
	protected:
		sprite_batch  _sprites;
	};

	class render_interface
	{
	public:
		using sprite_id = sprite_utility::sprite_id;
		using sprite_layer = sprite_utility::layer_t;

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

		drawable_id create_drawable();

		drawable_id create_drawable_ptr(const sf::Drawable*, sprite_layer);
		template<typename Drawable>
		drawable_id create_drawable_copy(Drawable, sprite_layer);

		void update_drawable_ptr(const sf::Drawable*, sprite_layer);
		template<typename Drawable>
		void update_drawable_copy(Drawable, sprite_layer);

		void destroy_drawable(drawable_id);

	private:
		drawable_id _create_drawable_any(std::any drawable, sprite_layer);
	};
}

#endif //HADES_RENDERINTERFACE_HPP
