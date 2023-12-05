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
		activated_object_view &get_objects() noexcept
		{
			const auto game_data_ptr = detail::get_game_data_ptr();
			return game_data_ptr->entity;
		}

		const resources::object* get_object(object_ref& o)
		{
			const auto game_data_ptr = detail::get_game_data_ptr();
			return state_api::get_object(o, *game_data_ptr->extra).object_type;
		}

		time_point get_last_time() noexcept
		{
			const auto game_data_ptr = detail::get_game_data_ptr();
			return game_data_ptr->current_time - game_data_ptr->dt;
		}

		time_duration get_delta_time() noexcept
		{
			const auto game_data_ptr = detail::get_game_data_ptr();
			return game_data_ptr->dt;
		}

		time_point get_time() noexcept
		{
			const auto game_data_ptr = detail::get_game_data_ptr();
			return game_data_ptr->current_time;
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
		void restore_level() noexcept
		{
			auto ptr = detail::get_game_data_ptr();
			detail::change_level(ptr->level_data, ptr->level_id);
			return;
		}

		unique_id current_level() noexcept
		{
			return detail::get_game_level_id();
		}

		void switch_level(unique_id id)
		{
			auto ptr = detail::get_game_data_ptr();
			detail::change_level(std::invoke(ptr->get_level, id), id);
			return;
		}

		void switch_to_mission() noexcept
		{
			auto ptr = detail::get_game_data_ptr();
			detail::change_level(ptr->mission_data, {});
			return;
		}

		world_rect_t get_world_bounds()
		{
			const auto ptr = detail::get_game_level_ptr();
			return ptr->get_world_bounds();
		}

		const terrain_map& get_world_terrain() noexcept
		{
			const auto game_data_ptr = detail::get_game_level_ptr();
			return game_data_ptr->get_world_terrain();
		}
	}

	namespace game::level::object
	{
		object_ref get_from_name(std::string_view n, time_point t)
		{
			auto ptr = hades::detail::get_game_level_ptr();
			return ptr->get_object_ref(n, t);
		}

		object_ref create(const object_instance& obj)
		{
			auto ptr = hades::detail::get_game_level_ptr();
			assert(obj.id == bad_entity);
			auto new_obj = ptr->create_object(obj, get_time());

			return new_obj;
		}

		object_ref clone(object_ref o)
		{
			auto ptr = hades::detail::get_game_level_ptr();
			return ptr->clone_object(o, get_time());
		}

		void destroy(object_ref e)
		{
			auto ptr = hades::detail::get_game_level_ptr();
			ptr->destroy_object(e, get_time());
			return;
		}

		void sleep_system(object_ref o, time_point t)
		{
			auto game_ptr = hades::detail::get_game_data_ptr();
			auto sys_ptr = hades::detail::get_game_systems_ptr();
			sys_ptr->sleep_entity(o, game_ptr->system, t);
			return;
		}

		time_point get_creation_time(object_ref o)
		{
			const auto game_data_ptr = hades::detail::get_game_level_ptr();
			auto& extra = game_data_ptr->get_extras();
			const auto& state = game_data_ptr->get_state();
			const auto& obj = state_api::get_object(o, extra);
			return state_api::get_object_creation_time(obj, state);
		}

		const tag_list& get_tags(object_ref o)
		{
			const auto game_data_ptr = hades::detail::get_game_level_ptr();
			auto& extra = game_data_ptr->get_extras();
			const auto& obj = state_api::get_object(o, extra);
			return resources::object_functions::get_tags(*obj.object_type);
		}

		bool is_alive(object_ref& o) noexcept
		{
			auto ptr = hades::detail::get_game_level_ptr();
			// NOTE: get_object_ptr returns nullptr for stale object refs
			return state_api::get_object_ptr(o, ptr->get_extras()) != nullptr;
		}

		bool is_alive(const object_ref& o) noexcept
		{
			auto obj = o;
			return is_alive(obj);
		}

		linear_curve<vec2_float>& get_position(object_ref o)
		{
			const auto pos_id = get_position_curve_id();
			return get_property_ref<linear_curve, vec2_float>(o, pos_id);
		}

		const vec2_float& get_size(const object_ref o)
		{
			const auto id = hades::get_size_curve_id();
			return get_property_ref<const_curve, vec2_float>(o, id);
		}

		step_curve<unique>& get_player_owner(object_ref o)
		{
			const auto id = hades::get_player_owner_id();
			return get_property_ref<step_curve, unique>(o, id);
		}

		const unique& get_collision_group(object_ref o)
		{
			const auto id = hades::get_collision_layer_curve_id();
			return get_property_ref<const_curve, unique>(o, id);
		}

		const collection_unique& get_move_layers(object_ref o)
		{
			const auto id = hades::get_move_layers_id();
			return get_property_ref<const_curve, collection_unique>(o, id);
		}

		const collection_float& get_move_values(object_ref o)
		{
			const auto id = hades::get_move_values_id();
			return get_property_ref<const_curve, collection_float>(o, id);
		}
	}

	namespace render
	{
		activated_object_view &get_objects() noexcept
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
		
		id_t create(const resources::animation* a, time_point t, layer_t l,
			vector2_float position, float r, vector2_float size, const resources::shader_uniform_map* u)
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			assert(render_data_ptr->render_output);
			return render_data_ptr->render_output->create_sprite(a, t, l, position, r, size, u);
		}

		static time_point prog_time(float p, const resources::animation* a)
		{
			return time_point{ time_cast<nanoseconds>(resources::animation_functions::get_duration(*a) * p) };
		}

		id_t create(const resources::animation* a, float progress, layer_t l,
			vector2_float position, float r, vector2_float size, const resources::shader_uniform_map* u)
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			assert(render_data_ptr->render_output);
			return render_data_ptr->render_output->create_sprite(a, prog_time(progress, a), l, position, r, size, u);
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

		void set(id_t id, const resources::animation* a, time_point t, layer_t l,
			vector2_float p, float r, vector2_float s, const resources::shader_uniform_map* u)
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			assert(render_data_ptr->render_output);
			render_data_ptr->render_output->set_sprite(id, a, t, l, p, r, s, u);
		}

		void set(id_t id, const resources::animation* a, float prog, layer_t l,
			vector2_float p, float r, vector2_float s, const resources::shader_uniform_map* u)
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			assert(render_data_ptr->render_output);
			render_data_ptr->render_output->set_sprite(id, a, prog_time(prog, a), l, p, r, s, u);
		}

		void set(id_t id, time_point t, vector2_float p, float r, vector2_float s)
		{
			auto render_data_ptr = detail::get_render_data_ptr();
			assert(render_data_ptr->render_output);
			render_data_ptr->render_output->set_sprite(id, t, p, r, s);
		}

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

		

		const resources::object* get_object_type(object_ref o) noexcept
		{
			const auto obj = state_api::get_object_ptr(o, *detail::get_render_extra_ptr());
			if (obj)
				return obj->object_type;
			return nullptr;
		}
	}

	namespace render::level::object
	{
		time_point get_creation_time(object_ref o)
		{
			const auto obj = state_api::get_object_ptr(o, *hades::detail::get_render_extra_ptr());
			return state_api::get_object_creation_time(*obj, hades::detail::get_render_level_ptr()->get_state());
		}

		const hades::linear_curve<vec2_float>& get_position(const object_ref o)
		{
			const auto id = hades::get_position_curve_id();
			return get_property_ref<hades::linear_curve, vec2_float>(o, id);
		}

		const vec2_float& get_size(const object_ref o)
		{
			const auto id = hades::get_size_curve_id();
			return get_property_ref<hades::const_curve, vec2_float>(o, id);
		}

		const resources::animation* get_animation(object_ref& o, unique_id id)
		{
			const auto& game_obj = state_api::get_object(o, *hades::detail::get_render_extra_ptr());
			return get_animation(*game_obj.object_type, id);
		}
	}
}
