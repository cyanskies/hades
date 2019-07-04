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
			const auto obj_curve = render::level::get_curve<unique>(ent, obj_type);
			const auto obj_id = obj_curve.get(time);

			const auto object = data::get<resources::object>(obj_id);
			const auto anims = get_editor_animations(*object);

			if (std::empty(anims))
				return {};

			const auto px = render::level::get_curve<float_t>(ent, x);
			const auto py = render::level::get_curve<float_t>(ent, y);

			const auto sx = render::level::get_curve<float_t>(ent, size_x);
			const auto sy = render::level::get_curve<float_t>(ent, size_y);
			
			return entity_info{
				hades::vector_float{ px.get(time), py.get(time) },
				hades::vector_float{ sx.get(time), sy.get(time) },
				random_element(std::begin(anims), std::end(anims))
			};
		}

		static const auto sprite_id_list = unique_id{};
		using sprite_id_t = std::map<entity_id, sprite_utility::sprite_id>;

		static void on_create()
		{
			assert(render::get_object() == bad_entity);
			render::create_system_value(sprite_id_list, sprite_id_t{});
		}

		static void on_connect()
		{
			const auto entity = render::get_object();
			assert(entity != bad_entity);
			auto dat = render::get_system_value<sprite_id_t>(sprite_id_list);

			assert(dat.find(entity) == std::end(dat));

			const auto ent = get_entity_info(entity);

			if (ent.anim == nullptr)
				return;

			const auto sprite_id = render::get_render_output().create_sprite(ent.anim,
				render::get_time(), render_interface::sprite_layer{},
				ent.position, ent.size);

			dat.emplace(entity, sprite_id);
			render::set_system_value(sprite_id_list, dat);

			return;
		}

		static void on_tick(hades::job_system&, hades::render_job_data& d)
		{
			/*const auto entity = d.entity;
			assert(entity != bad_entity);

			auto& dat = std::any_cast<system_data&>(d.system_data);

			if (dat.sprite_ids.exists(d.entity))
			{
				const auto ent = get_entity_info(d);
				const auto s_id = dat.sprite_ids.get(d.entity);
				d.render_output.set_position(s_id, ent.position);
				d.render_output.set_size(s_id, ent.size);
				d.render_output.set_animation(s_id, ent.anim, d.current_time);
			}*/

			return;
		}

		static void on_disconnect(hades::job_system&, hades::render_job_data& d)
		{
			const auto entity = d.entity;
			assert(entity != bad_entity);

			auto dat = render::get_system_value<sprite_id_t>(sprite_id_list);
			if (const auto s_id = dat.find(entity); s_id != std::end(dat))
				d.render_output.destroy_sprite(s_id->second);

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

	void set_game_data(system_job_data *d, bool async)
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

		time_point get_last_time()
		{
			assert(game_data_ptr);
			return game_data_ptr->current_time;
		}

		time_duration get_delta_time()
		{
			assert(game_data_ptr);
			return game_data_ptr->dt;
		}

		time_point get_time()
		{
			assert(game_data_ptr);
			return game_data_ptr->current_time + game_data_ptr->dt;
		}

		bool system_value_exists(unique_id key)
		{
			assert(game_data_ptr);
			return game_data_ptr->system_data.exists(key);
		}

		void destroy_system_value(unique_id key)
		{
			assert(game_data_ptr);
			game_data_ptr->system_data.erase(key);
		}

		void clear_system_values()
		{
			assert(game_data_ptr);
			game_data_ptr->system_data.clear();
		}
	}

	namespace game::level
	{
		object_ref create_object(object_instance obj)
		{
			auto ptr = detail::get_game_data_ptr();
			assert(ptr);
			assert(obj.id == bad_entity);
			obj.id = ptr->level_data->create_entity();

			const auto& o = game_new_objects.emplace_back(std::move(obj));
			return o.id;
		}
	}

	static thread_local render_job_data *render_data_ptr = nullptr;
	static thread_local transaction render_transaction{};
	static thread_local bool render_async = true;

	void set_render_data(render_job_data *j, bool async)
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

	render_interface& render::get_render_output()
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
		return render_data_ptr->system_data.exists(id);
	}

	void render::destroy_system_value(unique_id id)
	{
		assert(render_data_ptr);
		render_data_ptr->system_data.erase(id);
	}

	void render::clear_system_values()
	{
		assert(render_data_ptr);
		render_data_ptr->system_data.clear();
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
