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

		static entity_info get_entity_info(const hades::render_job_data& d)
		{
			const auto ent = d.entity;
			const auto& curves = d.level_data->get_curves();
			const auto [x, y] = get_position_curve_id();
			const auto [size_x, size_y] = get_size_curve_id();
			const auto obj_type = get_object_type_curve_id();
			const auto obj_curve = curves.unique_curves.get({ ent, obj_type });
			const auto obj_id = obj_curve.get(d.current_time);

			const auto object = data::get<resources::object>(obj_id);
			const auto anims = get_editor_animations(*object);

			if (std::empty(anims))
				return {};

			const auto px = curves.float_curves.get({ ent, x });
			const auto py = curves.float_curves.get({ ent, y });

			const auto sx = curves.float_curves.get({ ent, size_x });
			const auto sy = curves.float_curves.get({ ent, size_y });

			
			return entity_info{
				vector_float{ px.get(d.current_time), py.get(d.current_time) },
				vector_float{ sx.get(d.current_time), sy.get(d.current_time) },
				random_element(std::begin(anims), std::end(anims))
			};
		}

		static void on_create(hades::job_system&, hades::render_job_data& d)
		{
			assert(d.entity == bad_entity);
			//d.system_data = system_data{};
		}

		static void on_connect(hades::job_system&, hades::render_job_data& d)
		{
			const auto entity = d.entity;
			assert(entity != bad_entity);
			//auto &dat = std::any_cast<system_data&>(d.system_data);

			//assert(!dat.sprite_ids.exists(d.entity));

			const auto ent = get_entity_info(d);

			if (ent.anim == nullptr)
				return;

			const auto sprite_id = d.render_output.create_sprite(ent.anim,
				d.current_time, render_interface::sprite_layer{},
				ent.position, ent.size);

			//dat.sprite_ids.create(d.entity, sprite_id);
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

			/*auto& dat = std::any_cast<system_data&>(d.system_data);
			if (dat.sprite_ids.exists(d.entity))
			{
				const auto s_id = dat.sprite_ids.get(d.entity);
				d.render_output.destroy_sprite(s_id);
			}*/

			return;
		}

		static void on_destroy(hades::job_system&, hades::render_job_data& d)
		{
			assert(d.entity == bad_entity);
			//d.system_data.reset();
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

	void set_render_data(render_job_data *j, bool async)
	{
		assert(j);
		detail::render_data_ptr = j;
		detail::render_data_async = async;
	}

	void abort_render_job()
	{
		assert(detail::render_data_ptr);
		detail::render_transaction.abort();
		detail::render_data_ptr = nullptr;
	}

	void finish_render_job()
	{
		assert(detail::render_data_ptr);
		detail::render_transaction.commit();
		detail::render_data_ptr = nullptr;
	}
}