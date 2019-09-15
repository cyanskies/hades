#include "hades/game_system.hpp"

#include "hades/animation.hpp"
#include "hades/core_curves.hpp"
#include "hades/objects.hpp"
#include "hades/render_interface.hpp"
#include "hades/shared_map.hpp"

namespace hades
{
	//simple renderer, just displays the editor animation at the objects location
	namespace simple_renderer
	{
		using namespace std::string_view_literals;
		static constexpr auto name = "simple-renderer"sv;
		static unique_id id = unique_id::zero;

		static const auto sprite_id_list = unique_id{};
		using sprite_id_t = std::vector<std::pair<entity_id, sprite_utility::sprite_id>>;

		struct entity_info
		{
			vector_float position;
			vector_float size;
			const resources::animation* anim = nullptr;
		};

		static entity_info get_some_entity_info(resources::curve_types::object_ref e, time_point t)
		{
			const auto pos = get_position_curve_id();
			const auto size = get_size_curve_id();
			using namespace resources::curve_types;

			const auto px = render::level::get_value<vec2_float>({ e, pos }, t);

			const auto sx = render::level::get_value<vec2_float>({ e, size }, t);

			return entity_info{
				px,
				sx
			};
		}

		static entity_info get_entity_info(resources::curve_types::object_ref e, time_point t)
		{
			using namespace resources::curve_types;

			const auto obj_type = get_object_type_curve_id();
			const auto obj_id = render::level::get_value<unique>({ e, obj_type }, t);
			const auto object = data::get<resources::object>(obj_id);
			const auto anims = get_editor_animations(*object);

			if (std::empty(anims))
				return {};

			auto entity = get_some_entity_info(e, t);
			entity.anim = random_element(std::begin(anims), std::end(anims));
			return entity;
		}

		static sprite_utility::sprite_id find(const sprite_id_t& v, entity_id e)
		{
			for (const auto& x : v)
			{
				if (x.first == e)
					return x.second;
			}

			return sprite_utility::bad_sprite_id;
		}

		static void erase(sprite_id_t& v, entity_id e)
		{
			for (auto it = std::begin(v); it != std::end(v); ++it)
			{
				if (it->first == e)
				{
					*it = *std::rbegin(v);
					v.pop_back();
					return;
				}
			}

			return;
		}

		static void on_create()
		{
			render::set_system_data(sprite_id_t{});
			return;
		}

		static void on_connect()
		{
			const auto& ents = render::get_objects();

			auto &dat = render::get_system_data<sprite_id_t>();
			const auto time = render::get_time();
			for (const auto entity : ents)
			{
				const auto ent = get_entity_info(entity, time);

				if (ent.anim == nullptr)
					return;

				const auto sprite_id = render::get_render_output()->create_sprite(ent.anim,
					render::get_time(), render_interface::sprite_layer{},
					ent.position, ent.size);

				dat.emplace_back(entity, sprite_id);
			}

			//render::set_system_data(std::move(dat));
			return;
		}

		static void on_tick()
		{
			auto render_output = render::get_render_output();
			const auto& ents = render::get_objects();
			//TODO: use proper rendering interface functions
			const auto &dat = render::get_system_data<sprite_id_t>();
			const auto time = render::get_time();

			for (const auto entity : ents)
			{
				if (const auto sprite = find(dat, entity); sprite != sprite_utility::bad_sprite_id)
				{
					const auto ent = get_some_entity_info(entity, time);
					const auto& s_id = sprite;
					render_output->set_sprite(s_id, render::get_time(),
						ent.position, ent.size);
				}
			}
			return;
		}

		static void on_disconnect()
		{
			auto render_output = render::get_render_output();
			const auto& ents = render::get_objects();
			
			auto &dat = render::get_system_data<sprite_id_t>();
			for (const auto entity : ents)
			{
				if (const auto s_id = find(dat, entity); s_id != sprite_utility::bad_sprite_id)
					render_output->destroy_sprite(s_id);

				erase(dat, entity);
			}

			return;
		}

		static void on_destroy()
		{
			render::destroy_system_data();
			return;
		}
	}

	void register_game_system_resources(data::data_manager &d)
	{
		//register system script resources

		//register render system script resources

		//built in render systems

		simple_renderer::id = d.get_uid(simple_renderer::name);

		make_render_system(simple_renderer::id,
			simple_renderer::on_create,
			simple_renderer::on_connect,
			simple_renderer::on_disconnect,
			simple_renderer::on_tick,
			simple_renderer::on_destroy,
			d
		);
	}	

	static thread_local system_job_data* game_data_ptr = nullptr;
	static thread_local game_interface* game_current_level_ptr = nullptr;

	void set_game_data(system_job_data *d) noexcept
	{
		assert(d);
		game_data_ptr = d;
		game_current_level_ptr = d->level_data;
		return;
	}

	namespace game
	{
		const std::vector<object_ref> &get_objects() noexcept
		{
			assert(game_data_ptr);
			return game_data_ptr->entity;
		}

		time_point get_last_time() noexcept
		{
			assert(game_data_ptr);
			return game_data_ptr->prev_time;
		}

		time_duration get_delta_time() noexcept
		{
			assert(game_data_ptr);
			return game_data_ptr->dt;
		}

		time_point get_time() noexcept
		{
			assert(game_data_ptr);
			return game_data_ptr->prev_time + game_data_ptr->dt;
		}

		void destroy_system_data()
		{
			assert(game_data_ptr);
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
			assert(ptr);
			assert(obj.id == bad_entity);
			return ptr->level_data->create_entity(std::move(obj), get_time());
		}

		void destroy_object(object_ref)
		{

		}

		world_rect_t get_world_bounds()
		{
			const auto ptr = detail::get_game_data_ptr();
			return ptr->level_data->get_world_bounds();
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

		void set_position(object_ref o, world_unit_t x, world_unit_t y, time_point t)
		{
			static const auto curves = get_position_curve_id();
			set_value<world_vector_t>({ o, curves }, t, { x, y });
			return;
		}

		void set_position(object_ref o, world_unit_t x, world_unit_t y)
		{
			set_position(o, x, y, get_time());
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
	}

	static thread_local render_job_data *render_data_ptr = nullptr;

	void set_render_data(render_job_data *j) noexcept
	{
		assert(j);
		render_data_ptr = j;
		return;
	}

	namespace render
	{
		const std::vector<object_ref> &get_objects()
		{
			return render_data_ptr->entity;
		}

		render_interface* get_render_output()
		{
			assert(render_data_ptr);
			return render_data_ptr->render_output;
		}

		time_point get_time()
		{
			assert(render_data_ptr);
			return render_data_ptr->current_time;
		}

		void destroy_system_data()
		{
			assert(render_data_ptr);
			render_data_ptr->system_data->reset();
		}
	}

	namespace render::level
	{
		world_rect_t get_world_bounds()
		{
			return render_data_ptr->level_data->get_world_bounds();
		}
	}

	namespace detail
	{
		system_job_data* get_game_data_ptr()
		{
			assert(game_data_ptr);
			return game_data_ptr;
		}

		game_interface* get_game_level_ptr()
		{
			assert(game_current_level_ptr);
			return game_current_level_ptr;
		}

		render_job_data* get_render_data_ptr()
		{
			return render_data_ptr;
		}
	}
}
