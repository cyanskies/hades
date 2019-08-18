#include "hades/render_interface.hpp"

#include <algorithm>

#include "SFML/Graphics/RenderTarget.hpp"

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

	bool render_interface::sprite_exists(sprite_id id) const noexcept
	{
		return _sprite_batch.exists(id);
	}

	void render_interface::destroy_sprite(sprite_id id)
	{
		try
		{
			_sprite_batch.destroy_sprite(id);
		}
		catch (const sprite_batch_invalid_id & e)
		{
			throw render_interface_invalid_id{ e.what() };
		}

		return;
	}

	void render_interface::set_sprite(sprite_id id, const resources::animation* a, time_point t, sprite_layer l, vector_float position, vector_float size)
	{
		try
		{
			_sprite_batch.set_sprite(id, a, t, l, position, size);
		}
		catch (const sprite_batch_invalid_id& e)
		{
			throw render_interface_invalid_id{ e.what() };
		}
	}

	void render_interface::set_animation(sprite_id id, const resources::animation *a, time_point t)
	{
		try
		{
			_sprite_batch.set_animation(id, a, t);
		}
		catch (const sprite_batch_invalid_id & e)
		{
			throw render_interface_invalid_id{ e.what() };
		}

		return;
	}

	void render_interface::set_layer(sprite_id id, sprite_layer l)
	{
		try
		{
			_sprite_batch.set_layer(id, l);
		}
		catch (const sprite_batch_invalid_id& e)
		{
			throw render_interface_invalid_id{ e.what() };
		}

		return;
	}

	void render_interface::set_position(sprite_id id, vector_float p)
	{
		try
		{
			_sprite_batch.set_position(id, p);
		}
		catch (const sprite_batch_invalid_id& e)
		{
			throw render_interface_invalid_id{ e.what() };
		}	

		return;
	}

	void render_interface::set_size(sprite_id id, vector_float s)
	{
		try
		{
			_sprite_batch.set_size(id, s);
		}
		catch (const sprite_batch_invalid_id& e)
		{
			throw render_interface_invalid_id{ e.what() };
		}

		return;
	}

	using layer_size_type = std::vector<render_interface::object_layer>::size_type;
	using obj_size_type = std::vector<render_interface::drawable_object>::size_type;

	struct found_object
	{
		obj_size_type drawable_index;
		layer_size_type layer_index;
		render_interface::drawable_object* object = nullptr;
	};

	static found_object find_object(render_interface::drawable_id id,
		std::vector<render_interface::object_layer> &object_layers)
	{
		for (auto layer_index = layer_size_type{}; layer_index < std::size(object_layers); ++layer_index)
		{
			for (auto object_index = obj_size_type{};
				object_index < std::size(object_layers[layer_index].objects); ++object_index)
			{
				//NOTE: object ids don't change, so we can avoid locking
				// the object itself cannot be moved while we have the shared lock
				//const auto lock = std::lock_guard{ object.mutex };
				auto &object = object_layers[layer_index].objects[object_index];
				if (object.id == id)
					return { object_index, layer_index, &object };
			}
		}

		const auto message = "Cannot find drawable object with id: " + to_string(id);
		throw render_interface_invalid_id{ message };
	}

	render_interface::drawable_id render_interface::create_drawable()
	{
		return _create_drawable_any({}, nullptr, {});
	}

	static const sf::Drawable &get_ptr_from_any(const std::any& a)
	{
		return *std::any_cast<const sf::Drawable*>(a);
	}

	render_interface::drawable_id render_interface::create_drawable_ptr(const sf::Drawable *d, sprite_layer l)
	{
		return _create_drawable_any(d, get_ptr_from_any, l);
	}

	bool render_interface::drawable_exists(drawable_id id) const noexcept
	{
		return std::find(std::begin(_used_drawable_ids),
			std::end(_used_drawable_ids), id) != std::end(_used_drawable_ids);
	}

	void render_interface::update_drawable_ptr(drawable_id id, const sf::Drawable *d, sprite_layer l)
	{
		_update_drawable_any(id, d, get_ptr_from_any, l);
		return;
	}

	void render_interface::destroy_drawable(drawable_id id)
	{
		const auto obj = find_object(id, _object_layers);

		_destroy_id(id);
		_remove_from_layer(id, obj.layer_index, obj.drawable_index);
		return;
	}

	void render_interface::prepare()
	{
		_sprite_batch.apply();
	}

	void render_interface::draw(sf::RenderTarget &t, sf::RenderStates s) const
	{
		using sprite_utility::layer_t;

		t.draw(_sprite_batch, s);

		return;

		//TODO: make this less crazy

		constexpr auto layer_max = std::numeric_limits<layer_t>::max();
		constexpr auto layer_min = std::numeric_limits<layer_t>::min();

		auto drawable_iter = std::begin(_object_layers);
		const auto layer_ids = _sprite_batch.get_layer_list();
		auto sprite_iter = std::begin(layer_ids);

		//get the lowest layer, and the max layers for each tpe
		const auto [starting_layer, last_sprite_layer, last_object_layer] = [&]()->std::tuple<layer_t, layer_t, layer_t> {
			if (drawable_iter != std::end(_object_layers) &&
				sprite_iter != std::end(layer_ids))
				return { std::min(*sprite_iter, drawable_iter->layer), layer_ids.back(), _object_layers.back().layer };
			else if (drawable_iter == std::end(_object_layers) &&
				sprite_iter != std::end(layer_ids))
				return { *sprite_iter, layer_ids.back() , layer_min };
			else if (sprite_iter == std::end(layer_ids) &&
				drawable_iter != std::end(_object_layers))
				return { drawable_iter->layer, layer_min, _object_layers.back().layer };
			else
				return { layer_max, layer_min, layer_min };
		}();

		const auto final_layer = std::max(last_sprite_layer, last_object_layer);
		auto layer = starting_layer;

		while (layer <= final_layer)
		{
			if (drawable_iter != std::end(_object_layers) &&
				drawable_iter->layer == layer)
			{
				for (const auto& d : drawable_iter->objects)
					t.draw(d.get(d.storage), s);

				++drawable_iter;
			}

			if (sprite_iter != std::end(layer_ids) &&
				*sprite_iter == layer)
			{
				_sprite_batch.draw(t, layer, s);
				++sprite_iter;
			}

			++layer;
		}

		return;
	}

	render_interface::drawable_id render_interface::_make_new_id()
	{
		const auto id = increment(_drawable_id);

		//NOTE: if this is fired then we have used up all the ids
		//		will have to implement id reclamation strategy, or expand the id type
		assert(id != bad_drawable_id);

		_used_drawable_ids.emplace_back(id);

		return id;
	}

	void render_interface::_destroy_id(drawable_id id)
	{
		const auto iter = std::find(std::begin(_used_drawable_ids), std::end(_used_drawable_ids), id);
		if (iter == std::end(_used_drawable_ids))
		{
			const auto message = "Tried to delete an id that wasn't listed in the used id list, id was: " + to_string(id);
			throw render_interface_invalid_id{ message };
		}

		std::iter_swap(iter, std::rbegin(_used_drawable_ids));
		_used_drawable_ids.pop_back();
		return;
	}

	void render_interface::_add_to_layer(drawable_object obj, sprite_layer l)
	{
		using size_type = layer_size_type;
		constexpr auto no_index = std::numeric_limits<size_type>::max();

		auto layer_index = no_index;
		for (auto i = size_type{}; i < std::size(_object_layers); ++i)
		{
			if (_object_layers[i].layer == l)
			{
				layer_index = i;
				break;
			}
		}

		// Unable to find correct layer
		// so make it
		if (layer_index == no_index)
		{
			const auto end = std::end(_object_layers);
			auto iter = std::begin(_object_layers);

			for (auto iter2 = std::next(iter); iter != end; ++iter, ++iter2)
			{
				if (iter->layer > l)
					break;
				if ((iter->layer < l &&
					iter2 == end) ||
					(iter->layer < l &&
						iter2->layer > l))
				{
					iter = iter2;
					break;
				}
			}

			const auto location = _object_layers.emplace(iter, object_layer{ l, std::vector<drawable_object>{} });
			layer_index = std::distance(std::begin(_object_layers), location);
		}

		assert(layer_index < std::size(_object_layers));

		_object_layers[layer_index].objects.emplace_back(std::move(obj));
		return;
	}

	void render_interface::_remove_from_layer(drawable_id id,
		std::vector<object_layer>::size_type layer_index,
		std::vector<drawable_object>::size_type index) noexcept
	{
		//remove the object
		auto iter = std::begin(_object_layers[layer_index].objects);
		std::advance(iter, index);
		std::iter_swap(iter, std::rbegin(_object_layers[layer_index].objects));
		_object_layers[layer_index].objects.pop_back();
		return;
	}

	render_interface::drawable_id render_interface::_create_drawable_any(std::any drawable, get_drawable func, sprite_layer l)
	{	
		const auto id = _make_new_id();
		_add_to_layer({ id, std::move(drawable), func }, l);
		return id;
	}

	void render_interface::_update_drawable_any(
		drawable_id id, std::any drawable, get_drawable func, sprite_layer l)
	{

		auto object = find_object(id, _object_layers);

		if (l == _object_layers[object.layer_index].layer)
		{
			object.object->storage = std::move(drawable);
			object.object->get = func;
			return;
		}

		assert(object.object);
		auto obj = drawable_object{ std::move(*object.object) };

		_remove_from_layer(id, object.layer_index, object.drawable_index);
		_add_to_layer(std::move(obj), l);

		return;
	}
}