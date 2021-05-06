#include "hades/game_api.hpp"

#include "hades/animation.hpp"
#include "hades/core_curves.hpp"
#include "hades/game_system.hpp"
#include "hades/objects.hpp"
#include "hades/players.hpp"

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
			return game_data_ptr->current_time;
		}

		time_duration get_delta_time() noexcept
		{
			const auto game_data_ptr = detail::get_game_data_ptr();
			return game_data_ptr->dt;
		}

		time_point get_time() noexcept
		{
			const auto game_data_ptr = detail::get_game_data_ptr();
			return game_data_ptr->current_time + game_data_ptr->dt;
		}

		const std::vector<player_data>& get_players() noexcept
		{
			const auto ptr = detail::get_game_data_ptr();
			assert(ptr != nullptr); assert(ptr->players);
			return *ptr->players;
		}

		object_ref get_player(unique u) noexcept
		{
			for (const auto& p : get_players())
			{
				if (p.name == u)
					return p.player_object;
			}
			return bad_object_ref;
		}

		unique_id current_system() noexcept
		{
			const auto ptr = detail::get_game_data_ptr();
			return ptr->system;
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
		object_ref get_object_from_name(std::string_view n)
		{
			auto ptr = detail::get_game_level_ptr();
			return ptr->get_object_ref(n);
		}

		object_ref create_object(const object_instance& obj)
		{
			auto ptr = detail::get_game_level_ptr();
			assert(obj.id == bad_entity);
			auto new_obj = ptr->create_object(obj, get_time());

			return new_obj;
		}

		object_ref clone_object(object_ref o)
		{
			auto ptr = detail::get_game_level_ptr();
			return ptr->clone_object(o, get_time());
		}

		//TODO: do we need 'from', do we need 'untill' or just 'for'
		/*void sleep_system(object_ref e, unique_id s, time_point from, time_point until)
		{
			auto game_current_level_system_ptr = detail::get_game_systems_ptr();
			game_current_level_system_ptr->sleep_entity(e, s, from, until);
			return;
		}*/

		void destroy_object(object_ref e)
		{
			auto ptr = detail::get_game_level_ptr();
			ptr->destroy_object(e);
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

		time_point get_object_creation_time(object_ref o)
		{
			const auto game_data_ptr = detail::get_game_level_ptr();
			auto& extra = game_data_ptr->get_extras();
			const auto& state = game_data_ptr->get_state();
			const auto& obj = state_api::get_object(o, extra);
			return state_api::get_object_creation_time(obj, state);
		}

		world_vector_t& get_position(object_ref o)
		{
			const auto pos_id = get_position_curve_id();
			return get_property_ref<world_vector_t>(o, pos_id);
		}

		void set_position(object_ref o, world_vector_t v)
		{
			const auto pos_id = get_position_curve_id();
			set_property_value(o, pos_id, v);
			return;
		}

		world_vector_t& get_size(object_ref o)
		{
			const auto size = get_size_curve_id();
			return get_property_ref<world_vector_t>(o, size);
		}

		/*void set_size(object_ref o,const_curve<world_vector_t> v)
		{
			const auto size = get_size_curve_id();
			set_property_value(o, size, v);
			return;
		}*/

		tag_list& get_tags(object_ref o)
		{
			const auto tag_id = get_tags_curve_id();
			return get_property_ref<tag_list>(o, tag_id);
		}

		bool is_alive(object_ref& o) noexcept
		{
			auto ptr = detail::get_game_level_ptr();
			assert(ptr);
			// NOTE: get_object returns nullptr for stale object refs
			return state_api::get_object_ptr(o, ptr->get_extras()) != nullptr;
		}
	}

	namespace render
	{
		std::vector<object_ref> &get_objects() noexcept
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

		static time_point prog_time(float p, const resources::animation* a)
		{
			return time_point{ time_cast<nanoseconds>(a->duration * p) };
		}

		id_t create(const resources::animation* a, float progress, layer_t l, vector_float position, vector_float size)
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			assert(render_data_ptr->render_output);
			return render_data_ptr->render_output->create_sprite(a, prog_time(progress, a), l, position, size);
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

		void set(id_t id, const resources::animation* a, float prog, layer_t l, vector_float p, vector_float s)
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			assert(render_data_ptr->render_output);
			render_data_ptr->render_output->set_sprite(id, a, prog_time(prog, a), l, p, s);
		}

		void set(id_t id, time_point t, vector_float p, vector_float s)
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			assert(render_data_ptr->render_output);
			render_data_ptr->render_output->set_sprite(id, t, p, s);
		}

		/*void set(id_t id, float t, vector_float p, vector_float s)
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			assert(render_data_ptr->render_output);
			render_data_ptr->render_output->set_sprite(id, prog_time(t, a), p, s);
		}*/

		void set_animation(id_t id, const resources::animation* a, time_point t)
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			assert(render_data_ptr->render_output);
			render_data_ptr->render_output->set_animation(id, a, t);
		}

		void set_animation(id_t id, const resources::animation* a, float t)
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			assert(render_data_ptr->render_output);
			render_data_ptr->render_output->set_animation(id, a, prog_time(t, a));
		}

		void set_animation(id_t id, time_point t)
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			assert(render_data_ptr->render_output);
			render_data_ptr->render_output->set_animation(id, t);
		}
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

		world_vector_t get_position(object_ref o)
		{
			const auto pos_id = get_position_curve_id();
			return get_property_ref<world_vector_t>(o, pos_id);
		}

		world_vector_t get_size(object_ref o)
		{
			const auto size_id = get_size_curve_id();
			return get_property_ref<world_vector_t>(o, size_id);
		}

		time_point get_object_creation_time(object_ref o)
		{
			const auto obj = state_api::get_object_ptr(o, *detail::get_render_extra_ptr());
			return state_api::get_object_creation_time(*obj, detail::get_render_level_ptr()->get_state());
		}

		const resources::object* get_object_type(object_ref o) noexcept
		{
			const auto obj = state_api::get_object_ptr(o, *detail::get_render_extra_ptr());
			if (obj)
				return obj->object_type;
			return nullptr;
		}
	}
}
