#include "hades/game_api.hpp"

#include "hades/animation.hpp"
#include "hades/core_curves.hpp"
#include "hades/game_system.hpp"
#include "hades/objects.hpp"

namespace hades
{
	namespace game
	{
		const std::vector<object_ref> &get_objects() noexcept
		{
			const auto game_data_ptr = detail::get_game_data_ptr();
			return game_data_ptr->entity;
		}

		time_point get_last_time() noexcept
		{
			const auto game_data_ptr = detail::get_game_data_ptr();
			return game_data_ptr->prev_time;
		}

		time_duration get_delta_time() noexcept
		{
			const auto game_data_ptr = detail::get_game_data_ptr();
			return game_data_ptr->dt;
		}

		time_point get_time() noexcept
		{
			const auto game_data_ptr = detail::get_game_data_ptr();
			return game_data_ptr->prev_time + game_data_ptr->dt;
		}

		void destroy_system_data()
		{
			auto game_data_ptr = detail::get_game_data_ptr();
			assert(game_data_ptr->system_data);
			game_data_ptr->system_data->reset();
		}
	}

	namespace game::level
	{
		object_ref get_object_from_name(std::string_view n, time_point t)
		{
			auto ptr = detail::get_game_level_ptr();
			return ptr->get_entity_id(n, t);
		}

		object_ref create_object(object_instance obj)
		{
			//TODO: handle time better
			auto ptr = detail::get_game_data_ptr();
			assert(ptr && ptr->level_data);
			assert(obj.id == bad_entity);
			return ptr->level_data->create_entity(std::move(obj), get_time());
		}

		void attach_system(object_ref e, unique_id s, time_point t)
		{
			auto game_current_level_system_ptr = detail::get_game_level_ptr();
			game_current_level_system_ptr->attach_system(e, s, t);
			return;
		}

		void sleep_system(object_ref e, unique_id s, time_point from, time_point until)
		{
			auto game_current_level_system_ptr = detail::get_game_systems_ptr();
			game_current_level_system_ptr->sleep_entity(e, s, from, until);
			return;
		}

		void detach_system(object_ref e, unique_id s, time_point t)
		{
			auto game_current_level_system_ptr = detail::get_game_systems_ptr();
			game_current_level_system_ptr->detach_system(e, s, t);
			return;
		}

		void destroy_object(object_ref e, time_point t)
		{
			auto game_current_level_ptr = detail::get_game_level_ptr();
			auto game_current_level_system_ptr = detail::get_game_systems_ptr();
			const auto alive_id = get_alive_curve_id();
			detail::set_game_value(game_current_level_ptr, { e, alive_id }, t, false);
			game_current_level_system_ptr->detach_all(e, t);
			return;
		}

		world_rect_t get_world_bounds()
		{
			const auto ptr = detail::get_game_data_ptr();
			return ptr->level_data->get_world_bounds();
		}

		const terrain_map& get_world_terrain() noexcept
		{
			const auto game_data_ptr = detail::get_game_data_ptr();
			return game_data_ptr->level_data->get_world_terrain();
		}

		world_vector_t get_position(object_ref o, time_point t)
		{
			//TODO: is it ok to cache the curve ptrs
			static const auto curves = get_position_curve_id();
			return get_value<world_vector_t>(curve_index_t{ o, curves }, t);
		}

		world_vector_t get_position(object_ref o)
		{
			return get_position(o, game::get_last_time());
		}

		void set_position(object_ref o, world_vector_t v, time_point t)
		{
			static const auto curves = get_position_curve_id();
			set_value({ o, curves }, t, v);
			return;
		}

		void set_position(object_ref o, world_vector_t v)
		{
			set_position(o, v, get_time());
			return;
		}

		world_vector_t get_size(object_ref o, time_point t)
		{
			static const auto curve = get_size_curve_id();
			return get_value<world_vector_t>({ o, curve }, t);
		}

		world_vector_t get_size(object_ref o)
		{
			return get_size(o, get_last_time());
		}

		void set_size(object_ref o, world_unit_t w, world_unit_t h, time_point t)
		{
			static const auto curves = get_size_curve_id();
			set_value<world_vector_t>({ o, curves }, { w, h });
			return;
		}

		void set_size(object_ref o, world_vector_t v, time_point t)
		{
			const auto size = get_size_curve_id();
			set_value({ o, size }, t, v);
			return;
		}

		void set_size(object_ref o, world_vector_t v)
		{
			set_size(o, v, get_time());
			return;
		}

		bool tags(object_ref o, tag_list has, tag_list not)
		{
			const auto& tags = get_value<tag_list>({ o, get_tags_curve_id() });
			bool has1 = false, has2 = false;

			for (const auto& t : tags)
			{
				const auto eq = [t](tag_t t2) noexcept {return t == t2; };
				if (std::any_of(begin(has), end(has), eq))
					has1 = true;
				if (std::any_of(begin(not), end(not), eq))
					has2 = true;
			}

			return has1 && !has2;
		}
	}

	namespace render
	{
		const std::vector<object_ref> &get_objects() noexcept
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			return render_data_ptr->entity;
		}

		render_interface* get_render_output() noexcept
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			return render_data_ptr->render_output;
		}

		time_point get_time() noexcept
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			return render_data_ptr->current_time;
		}

		void destroy_system_data() noexcept
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			render_data_ptr->system_data->reset();
		}
	}

	namespace render::sprite
	{
		id_t create()
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			assert(render_data_ptr->render_output);
			return render_data_ptr->render_output->create_sprite();
		}
		
		id_t create(const resources::animation* a, time_point t, layer_t l, vector_float position, vector_float size)
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			assert(render_data_ptr->render_output);
			return render_data_ptr->render_output->create_sprite(a, t, l, position, size);
		}

		void destroy(id_t id)
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			assert(render_data_ptr->render_output);
			return render_data_ptr->render_output->destroy_sprite(id);
		}

		bool exists(id_t id) noexcept
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			assert(render_data_ptr && render_data_ptr->render_output);
			return render_data_ptr->render_output->sprite_exists(id);
		}

		void set(id_t id, const resources::animation* a, time_point t, layer_t l, vector_float p, vector_float s)
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			assert(render_data_ptr->render_output);
			render_data_ptr->render_output->set_sprite(id, a, t, l, p, s);
		}

		void set(id_t id, time_point t, vector_float p, vector_float s)
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			assert(render_data_ptr->render_output);
			render_data_ptr->render_output->set_sprite(id, t, p, s);
		}

		/*void set_animation(id_t id, const resources::animation* a, time_point t)
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			assert(render_data_ptr->render_output);
			render_data_ptr->render_output->set_animation(id, a, t);
		}

		void set_layer(id_t id, layer_t l)
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			assert(render_data_ptr->render_output);
			render_data_ptr->render_output->set_layer(id, l);
		}

		void set_size(id_t id, vector_float s)
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			assert(render_data_ptr->render_output);
			render_data_ptr->render_output->set_size(id, s);
		}*/
	}

	namespace render::drawable
	{
		id_t create()
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			assert(render_data_ptr->render_output);
			return render_data_ptr->render_output->create_drawable();
		}

		id_t create_ptr(const sf::Drawable* d, layer_t l)
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			assert(render_data_ptr->render_output);
			return render_data_ptr->render_output->create_drawable_ptr(d, l);
		}

		bool exists(id_t id) noexcept
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			assert(render_data_ptr && render_data_ptr->render_output);
			return render_data_ptr->render_output->drawable_exists(id);
		}
		
		void update_ptr(id_t id, const sf::Drawable* d, layer_t l)
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			assert(render_data_ptr->render_output);
			render_data_ptr->render_output->update_drawable_ptr(id, d, l);
		}

		void destroy(id_t id)
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			assert(render_data_ptr->render_output);
			return render_data_ptr->render_output->destroy_drawable(id);
		}
	}

	namespace render::level
	{
		world_rect_t get_world_bounds()
		{
			const auto render_data_ptr = detail::get_render_data_ptr();
			return render_data_ptr->level_data->get_world_bounds();
		}

		const terrain_map& get_world_terrain() noexcept
		{
			const auto render_data_ptr = detail::get_render_data_ptr();
			return render_data_ptr->level_data->get_world_terrain();
		}

		world_vector_t get_position(object_ref o, time_point t)
		{
			const auto pos_id = get_position_curve_id();
			return get_value<world_vector_t>({ o, pos_id }, t);
		}

		world_vector_t get_position(object_ref o)
		{ return get_position(o, get_time()); }

		world_vector_t get_size(object_ref o, time_point t)
		{
			const auto size_id = get_size_curve_id();
			return get_value<world_vector_t>({ o, size_id }, t);
		}

		world_vector_t get_size(object_ref o)
		{ return get_size(o, get_time()); }
	}
}
