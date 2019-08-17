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

		struct entity_info
		{
			vector_float position;
			vector_float size;
			const resources::animation* anim = nullptr;
		};

		static entity_info get_entity_info(resources::curve_types::object_ref e)
		{
			using namespace resources::curve_types;

			const auto time = render::get_time();
			const auto ent = render::get_object();
			const auto [x, y] = get_position_curve_id();
			const auto [size_x, size_y] = get_size_curve_id();
			const auto obj_type = get_object_type_curve_id();
			const auto obj_id = render::level::get_value<unique>({ ent, obj_type }, time);
			const auto object = data::get<resources::object>(obj_id);
			const auto anims = get_editor_animations(*object);

			if (std::empty(anims))
				return {};

			const auto px = render::level::get_value<float_t>({ ent, x }, time);
			const auto py = render::level::get_value<float_t>({ ent, y }, time);

			const auto sx = render::level::get_value<float_t>({ ent, size_x }, time);
			const auto sy = render::level::get_value<float_t>({ ent, size_y }, time);
			
			return entity_info{
				hades::vector_float{ px, py },
				hades::vector_float{ sx, sy },
				random_element(std::begin(anims), std::end(anims))
			};
		}

		static const auto sprite_id_list = unique_id{};
		using sprite_id_t = std::vector<std::pair<entity_id, sprite_utility::sprite_id>>;

		sprite_utility::sprite_id find(const sprite_id_t& v, entity_id e)
		{
			for (const auto& x : v)
			{
				if (x.first == e)
					return x.second;
			}

			return sprite_utility::bad_sprite_id;
		}

		void erase(sprite_id_t& v, entity_id e)
		{
			for (auto it = std::begin(v); it != std::end(v); ++it)
			{
				if (it->first == e)
				{
					std::iter_swap(it, std::rbegin(v));
					v.pop_back();
					return;
				}
			}

			return;
		}

		static void on_create()
		{
			assert(render::get_object() == bad_entity);
			render::create_system_value(sprite_id_list, sprite_id_t{});
			return;
		}

		static void on_connect()
		{
			const auto entity = render::get_object();
			assert(entity != bad_entity);
			auto dat = render::get_system_value<sprite_id_t>(sprite_id_list);

			const auto ent = get_entity_info(entity);

			if (ent.anim == nullptr)
				return;

			const auto sprite_id = render::get_render_output()->create_sprite(ent.anim,
				render::get_time(), render_interface::sprite_layer{},
				ent.position, ent.size);

			dat.emplace_back(entity, sprite_id);
			render::set_system_value(sprite_id_list, std::move(dat));

			return;
		}

		static void on_tick(hades::render_job_data& d)
		{
			const auto entity = d.entity;
			assert(entity != bad_entity);

			//TODO: use proper rendering interface functions
			const auto dat = render::get_system_value<sprite_id_t>(sprite_id_list);
			if (const auto sprite = find(dat, entity); sprite != sprite_utility::bad_sprite_id)
			{
				const auto ent = get_entity_info(entity);
				const auto &s_id = sprite;
				d.render_output->set_sprite(s_id, ent.anim, render::get_time(),
					render_interface::sprite_layer{}, ent.position, ent.size);
			}

			return;
		}

		static void on_disconnect(hades::job_system&, hades::render_job_data& d)
		{
			const auto entity = d.entity;
			assert(entity != bad_entity);

			auto dat = render::get_system_value<sprite_id_t>(sprite_id_list);
			if (const auto s_id = find(dat, entity); s_id != sprite_utility::bad_sprite_id)
				d.render_output->destroy_sprite(s_id);

			erase(dat, entity);
			render::set_system_value(sprite_id_list, std::move(dat));

			return;
		}

		static void on_destroy()
		{
			assert(render::get_object() == bad_entity);
			render::clear_system_values();
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

	resources::curve_types::vector_object_ref get_added_entites(const name_list& nl_curve, time_point last_frame, time_point this_frame)
	{
		assert(!std::empty(nl_curve));
		auto prev = nl_curve.get(last_frame);
		auto next = nl_curve.get(this_frame);

		std::sort(std::begin(prev), std::end(prev));
		std::sort(std::begin(next), std::end(next));

		auto output = resources::curve_types::vector_object_ref{};

		std::set_difference(std::begin(next), std::end(next),
			std::begin(prev), std::end(prev),
			std::back_inserter(output));

		return output;
	}

	resources::curve_types::vector_object_ref get_removed_entites(const name_list &nl_curve, time_point last_frame, time_point this_frame)
	{
		assert(!std::empty(nl_curve));
		auto prev = nl_curve.get(last_frame);
		auto next = nl_curve.get(this_frame);

		std::sort(std::begin(prev), std::end(prev));
		std::sort(std::begin(next), std::end(next));

		auto output = resources::curve_types::vector_object_ref{};

		std::set_difference(std::begin(prev), std::end(prev),
			std::begin(next), std::end(next),
			std::back_inserter(output));

		return output;
	}

	static thread_local system_job_data* game_data_ptr = nullptr;
	static thread_local game_interface* game_current_level_ptr = nullptr;
	static thread_local transaction game_transaction{};
	static thread_local bool game_async = true;
	static thread_local std::vector<object_instance> game_new_objects;

	void set_game_data(system_job_data *d, bool async) noexcept
	{
		assert(d);
		game_data_ptr = d;
		game_current_level_ptr = d->level_data;
		game_async = async;
		return;
	}

	void abort_game_job()
	{
		assert(game_data_ptr);
		game_data_ptr = nullptr;
		game_transaction.abort();
		return;
	}

	bool finish_game_job()
	{
		assert(game_data_ptr);
		const auto commit_success = game_transaction.commit();

		if (commit_success)
		{
			//TODO: store ptr to the level the object was created for
			// currently we can only create objects in the level the system is running in
			for (const auto& o : game_new_objects)
				game_data_ptr->level_data->create_entity(o, game::get_time());
		}

		game_new_objects.clear();

		game_data_ptr = nullptr;
		game_current_level_ptr = nullptr;
		return commit_success;
	}

	namespace game
	{
		object_ref get_object() noexcept
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

		bool system_value_exists(unique_id key)
		{
			assert(game_data_ptr);
			return game_data_ptr->system_data->exists(key);
		}

		void destroy_system_value(unique_id key)
		{
			assert(game_data_ptr);
			game_data_ptr->system_data->erase(key);
		}

		void clear_system_values()
		{
			assert(game_data_ptr);
			game_data_ptr->system_data->clear();
		}
	}

	namespace game::level
	{
		object_ref create_object(object_instance obj)
		{
			//TODO: handle time better
			auto ptr = detail::get_game_data_ptr();
			assert(ptr);
			assert(obj.id == bad_entity);
			if (detail::get_game_data_async())
			{
				obj.id = ptr->level_data->create_entity();

				const auto& o = game_new_objects.emplace_back(std::move(obj));
				return o.id;
			}
			else
				return ptr->level_data->create_entity(std::move(obj), get_time());
		}

		world_vector_t get_position(object_ref o, time_point t)
		{
			//TODO: is it ok to cache the curve ptrs
			static const auto curves = get_position_curve_id();
			return {
				get_value<world_unit_t>(o, std::get<0>(curves), t),
				get_value<world_unit_t>(o, std::get<1>(curves), t)
			};
		}

		world_vector_t get_position(object_ref o)
		{
			return get_position(o, game::get_last_time());
		}

		world_vector_t get_position()
		{
			return get_position(get_object());
		}

		void set_position(object_ref o, world_unit_t x, world_unit_t y, time_point t)
		{
			static const auto curves = get_position_curve_id();
			set_value(o, std::get<0>(curves), t, x);
			set_value(o, std::get<1>(curves), t, y);
			return;
		}

		void set_position(object_ref o, world_unit_t x, world_unit_t y)
		{
			set_position(o, x, y, get_time());
			return;
		}

		void set_position(world_unit_t x, world_unit_t y)
		{
			set_position(get_object(), x, y);
			return;
		}

		world_vector_t get_size(object_ref o, time_point t)
		{
			static const auto curve = get_size_curve_id();
			return {
				get_value<world_unit_t>(o, std::get<0>(curve), t),
				get_value<world_unit_t>(o, std::get<1>(curve), t)
			};
		}

		world_vector_t get_size(object_ref o)
		{
			return get_size(o, get_last_time());
		}

		world_vector_t get_size()
		{
			return get_size(get_object());
		}

		void set_size(object_ref o, world_unit_t w, world_unit_t h, time_point t)
		{
			static const auto curves = get_size_curve_id();
			set_value(o, std::get<0>(curves), w);
			set_value(o, std::get<1>(curves), h);
			return;
		}
		
		void set_size(object_ref o, world_unit_t w, world_unit_t h)
		{
			set_size(o, w, h, get_time());
			return;
		}
		
		void set_size(world_unit_t w, world_unit_t h)
		{
			set_size(get_object(), w, h);
		}
	}

	static thread_local render_job_data *render_data_ptr = nullptr;
	static thread_local transaction render_transaction{};
	static thread_local bool render_async = true;

	void set_render_data(render_job_data *j, bool async) noexcept
	{
		assert(j);
		render_data_ptr = j;
		render_async = async;
		return;
	}

	void abort_render_job()
	{
		assert(render_data_ptr);
		render_transaction.abort();
		render_data_ptr = nullptr;
		return;
	}

	bool finish_render_job()
	{
		assert(render_data_ptr);
		render_data_ptr = nullptr;
		return render_transaction.commit();
	}

	resources::curve_types::object_ref render::get_object()
	{
		assert(render_data_ptr);
		return render_data_ptr->entity;
	}

	render_interface *render::get_render_output()
	{
		assert(render_data_ptr);
		return render_data_ptr->render_output;
	}

	time_point render::get_time()
	{
		assert(render_data_ptr);
		return render_data_ptr->current_time;
	}

	bool render::system_value_exists(unique_id id)
	{
		assert(render_data_ptr);
		return render_data_ptr->system_data->exists(id);
	}

	void render::destroy_system_value(unique_id id)
	{
		assert(render_data_ptr);
		render_data_ptr->system_data->erase(id);
	}

	void render::clear_system_values()
	{
		assert(render_data_ptr);
		render_data_ptr->system_data->clear();
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

		transaction& get_game_transaction()
		{
			return game_transaction;
		}

		bool get_game_data_async()
		{
			return game_async;
		}

		render_job_data* get_render_data_ptr()
		{
			return render_data_ptr;
		}

		transaction& get_render_transaction()
		{
			return render_transaction;
		}
		
		bool get_render_data_async()
		{
			return render_async;
		}
	}
}
