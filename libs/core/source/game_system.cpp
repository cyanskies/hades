#include "hades/game_system.hpp"

#include "hades/animation.hpp"
#include "hades/core_curves.hpp"
#include "hades/game_api.hpp"
#include "hades/objects.hpp"

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

		static entity_info get_some_entity_info(resources::curve_types::object_ref e)
		{
			return entity_info{
				render::level::get_position(e),
				render::level::get_size(e)
			};
		}

		static entity_info get_entity_info(resources::curve_types::object_ref e)
		{
			assert(e.ptr && e.ptr->object_type);
			const auto object = e.ptr->object_type;
			const auto anims = get_editor_animations(*object);

			if (std::empty(anims))
				return {};

			auto entity = get_some_entity_info(e);
			entity.anim = random_element(std::begin(anims), std::end(anims));
			return entity;
		}

		static sprite_utility::sprite_id find(const sprite_id_t& v, entity_id e) noexcept
		{
			for (const auto& x : v)
			{
				if (x.first == e)
					return x.second;
			}

			return sprite_utility::bad_sprite_id;
		}

		static void erase(sprite_id_t& v, entity_id e) noexcept
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
				const auto ent = get_entity_info(entity);

				if (ent.anim == nullptr)
					return;

				const auto sprite_id = render::get_render_output()->create_sprite(ent.anim,
					render::get_time(), render_interface::sprite_layer{},
					ent.position, ent.size);

				dat.emplace_back(entity.id, sprite_id);
			}

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
				if (const auto sprite = find(dat, entity.id); sprite != sprite_utility::bad_sprite_id)
				{
					const auto ent = get_some_entity_info(entity);
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
				if (const auto s_id = find(dat, entity.id); s_id != sprite_utility::bad_sprite_id)
					render_output->destroy_sprite(s_id);

				erase(dat, entity.id);
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

	static system_job_data* game_data_ptr = nullptr;
	static game_interface* game_current_level_ptr = nullptr;
	static system_behaviours<game_system>* game_current_level_system_ptr = nullptr;

	void set_game_data(system_job_data *d) noexcept
	{
		assert(d);
		game_data_ptr = d;
		game_current_level_ptr = d->level_data;
		game_current_level_system_ptr = d->systems;
		return;
	}

	static render_job_data *render_data_ptr = nullptr;

	void set_render_data(render_job_data *j) noexcept
	{
		assert(j);
		render_data_ptr = j;
		return;
	}

	namespace detail
	{
		system_job_data* get_game_data_ptr() noexcept
		{
			assert(game_data_ptr);
			return game_data_ptr;
		}

		game_interface* get_game_level_ptr() noexcept
		{
			assert(game_current_level_ptr);
			return game_current_level_ptr;
		}

		system_behaviours<game_system>* get_game_systems_ptr() noexcept
		{
			assert(game_current_level_system_ptr);
			return game_current_level_system_ptr;
		}

		render_job_data* get_render_data_ptr() noexcept
		{
			return render_data_ptr;
		}

		common_interface* get_render_level_ptr() noexcept
		{
			return render_data_ptr->level_data;
		}

		extra_state<render_system>* get_render_extra_ptr() noexcept
		{
			return render_data_ptr->extra;
		}
	}
}
