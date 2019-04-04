#include "hades/render_interface.hpp"

namespace hades 
{
	render_interface::sprite_id render_interface::create_sprite()
	{
		return _sprite_batch.create_sprite();
	}
	
	render_interface::sprite_id render_interface::create_sprite(const resources::animation *a,
		time_point t, sprite_layer l, vector_float p, vector_float s)
	{
		return _sprite_batch.create_sprite(a, t, l, p, s);
	}

	bool render_interface::sprite_exists(sprite_id id) const
	{
		return _sprite_batch.exists(id);
	}

	void render_interface::destroy_sprite(sprite_id id)
	{
		_sprite_batch.destroy_sprite(id);
	}

	void render_interface::set_animation(sprite_id id, const resources::animation *a, time_point t)
	{
		_sprite_batch.set_animation(id, a, t);
	}

	void render_interface::set_layer(sprite_id id, sprite_layer l)
	{
		_sprite_batch.set_layer(id, l);
	}

	void render_interface::set_position(sprite_id id, vector_float p)
	{
		_sprite_batch.set_position(id, p);
	}

	void render_interface::set_size(sprite_id id, vector_float s)
	{
		_sprite_batch.set_size(id, s);
	}

	struct found_object
	{
		render_interface::drawable_object* object;
	};

	found_object find_object(render_interface::drawable_id id,
		std::vector<render_interface::object_layer> &object_layers)
	{
		for (auto& layer : object_layers)
			for (auto& object : layer.objects)
				if (object.id == id)
					return found_object{ &object };
	}
}