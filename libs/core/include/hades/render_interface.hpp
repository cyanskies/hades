#ifndef HADES_RENDER_INTERFACE_HPP
#define HADES_RENDER_INTERFACE_HPP

#include <any>
#include <mutex>
#include <vector>

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

	class render_interface
	{
	public:
		using sprite_id = sprite_utility::sprite_id;
		using sprite_layer = sprite_utility::layer_t;


		struct drawable_id_tag {};
		using drawable_id = strong_typedef<drawable_id_tag, int32>;
		using drawable_layer = sprite_layer;

		static constexpr drawable_id bad_drawable_id = drawable_id{ std::numeric_limits<drawable_id::value_type>::min() };

		using get_drawable = const sf::Drawable& (*)(std::any&);

		struct drawable_object
		{
			drawable_object() = default;

			drawable_object(drawable_id i, std::any a, get_drawable f) noexcept
				: id{ i }, storage{ std::move(a) }, get{ f }
			{}

			drawable_object(const drawable_object& rhs)
				: id{ rhs.id }, storage{ rhs.storage }, get{ rhs.get }
			{}

			drawable_object(drawable_object&& rhs) noexcept
				: id{ rhs.id }, storage{ std::move(rhs) }, get{ rhs.get }
			{}

			drawable_object& operator=(const drawable_object& rhs)
			{
				id = rhs.id;
				storage = rhs.storage;
				get = rhs.get;
			}

			drawable_object& operator=(drawable_object&& rhs) noexcept
			{
				id = rhs.id;
				storage = std::move(rhs.storage);
				get = rhs.get;
			}

			drawable_id id = bad_drawable_id;
			std::any storage;
			get_drawable get = nullptr;
			std::mutex mutex;
		};

		struct object_layer
		{
			object_layer() = default;
			object_layer(sprite_layer l, std::vector<drawable_object> v) noexcept
				: layer{ l }, objects{ std::move(v) }
			{}

			sprite_layer layer;
			std::vector<drawable_object> objects;
		};

		static_assert(std::is_nothrow_move_assignable_v<object_layer>);

		//===Thread-Safe===

		sprite_id create_sprite();
		sprite_id create_sprite(const resources::animation *, time_point,
			sprite_layer, vector_float position, vector_float size);

		bool sprite_exists(sprite_id) const noexcept;
		//NOTE: functions that accept sprite_id or drawable_id can throw render_instance_invalid_id
		void destroy_sprite(sprite_id);

		void set_animation(sprite_id, const resources::animation *, time_point);
		void set_layer(sprite_id, sprite_layer);
		void set_position(sprite_id, vector_float position);
		void set_size(sprite_id, vector_float size);

		drawable_id create_drawable();

		drawable_id create_drawable_ptr(const sf::Drawable*, sprite_layer);
		template<typename Drawable>
		drawable_id create_drawable_copy(Drawable, sprite_layer);

		bool drawable_exists(drawable_id) const noexcept;

		void update_drawable_ptr(drawable_id, const sf::Drawable*, sprite_layer);
		template<typename Drawable>
		void update_drawable_copy(drawable_id, Drawable, sprite_layer);

		void destroy_drawable(drawable_id);

		//===End Thread-Safe===

	private:
		drawable_id _make_new_id();
		void _destroy_id(drawable_id);
		void _add_to_layer(drawable_object, sprite_layer);
		void _remove_from_layer(drawable_id, std::vector<object_layer>::size_type layer_index, std::vector<drawable_object>::size_type obj_index);
		drawable_id _create_drawable_any(std::any drawable, get_drawable, sprite_layer);

		//sprite batch is already thread safe
		sprite_batch _sprite_batch;

		mutable std::shared_mutex _drawable_id_mutex;
		std::vector<drawable_id> _used_drawable_ids;
		drawable_id _drawable_id = bad_drawable_id;

		mutable std::shared_mutex _object_mutex;
		std::vector<object_layer> _object_layers;
	};

	template<typename Drawable>
	inline render_interface::drawable_id render_interface::create_drawable_copy(Drawable d, sprite_layer l)
	{
		return _create_drawable_any(d, [](std::any & a)->const sf::Drawable& {
				return std::any_cast<Drawable>(a);
			}, l);
	}
}

#endif //HADES_RENDER_INTERFACE_HPP
