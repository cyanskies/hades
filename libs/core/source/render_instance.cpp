#include "hades/render_instance.hpp"

#include "hades/core_curves.hpp"

namespace hades 
{
	struct empty_struct_t {};

	constexpr static auto empty_struct = empty_struct_t{};

	template<typename T, typename Callback = empty_struct_t>
	static void merge_input(const std::vector<exported_curves::export_set<T>>& input, curve_data& output, Callback callback = empty_struct)
	{
		auto& output_curves = get_curve_list<T>(output);
		for (const auto& [ent, var, frames] : input)
		{
			const auto id = std::decay_t<decltype(output_curves)>::key_type{ ent, var };
			const auto exists = output_curves.exists(id);
			auto new_curve = [exists, id, &output_curves]() {
				if (exists)
					return output_curves.get(id);
				else
				{
					const auto c = data::get<resources::curve>(id.second);
					return curve<T>{ c->c_type };
				}
			}();

			for (auto& f : frames)
				new_curve.set(f.first, f.second);

			if (exists)
				output_curves.set(id, new_curve);
			else
			{
				output_curves.create(id, new_curve);

				//NOTE: special case to check unique curves for the object-type curve
				if constexpr (std::is_same_v<T, resources::curve_types::unique>)
				{
					static_assert(std::is_invocable_v<Callback, entity_id, unique_id, time_point>,
						"Callback must accept an entity id, object_type id and time point");
					if (var == get_object_type_curve_id())
					{
						//the object type curve is a const type, 
						//it should only ever have a single keyframe
						assert(std::size(frames) == 1);

						// the time_point of obj-type should be 
						// the creation time of the entity
						const auto [t, type] = frames[0];

						//use callback to request object setup
						std::invoke(callback, ent, type, t);
					}
				}
			}
		}
	}

	void setup_systems_for_new_object(entity_id e, unique_id obj_type,
		time_point t, render_implementation& game)
	{
		using namespace std::string_literals;

		//TODO: implement to_string for timer types
		const resources::object *o = nullptr;
		try
		{
			o = data::get<resources::object>(obj_type);
		}
		catch (const data::resource_null &e)
		{
			const auto msg = "Cannot create client instance of entity, object-type("s
				+ to_string(obj_type) + ") is not associated with a resource."s 
				+ " Gametime: "s + /*to_string(t) +*/ ". "s + e.what();
			LOGERROR(msg);
			return;
		}
		catch (const data::resource_wrong_type &e)
		{
			const auto msg = "Cannot create client instance of entity, object-type("s
				+ to_string(obj_type) + ") is not an object resource"s
				+ " Gametime: "s + /*to_string(t) +*/ ". "s + e.what();
			LOGERROR(msg);
			return;
		}

		const auto systems = get_render_systems(*o);

		for (const auto s : systems)
			game.attach_system(e, s->id, t);
	}

	void render_instance::input_updates(const exported_curves& input)
	{
		auto& curves = _game.get_curves();

		merge_input(input.int_curves, curves);
		merge_input(input.float_curves, curves);
		merge_input(input.bool_curves, curves);
		merge_input(input.string_curves, curves);
		merge_input(input.object_ref_curves, curves);

		auto obj_type_callback = [game = &_game](entity_id e, unique_id u, time_point t){
			setup_systems_for_new_object(e, u, t, *game);
		};

		merge_input(input.unique_curves, curves, obj_type_callback);

		merge_input(input.int_vector_curves, curves);
		merge_input(input.float_vector_curves, curves);
		merge_input(input.object_ref_vector_curves, curves);
		merge_input(input.unique_vector_curves, curves);
	}

	void render_instance::make_frame_at(time_point t, render_implementation *m, render_interface &i)
	{
		assert(_jobs.ready());
		const auto on_create_parent = _jobs.create();
		const auto new_systems = _game.get_new_systems();

		std::vector<job_system::job*> jobs;
		for (const auto s : new_systems)
		{
			const auto j = _jobs.create_child(on_create_parent, s->on_create, render_job_data{
				 bad_entity, &_game, m, t, i, _game.get_system_data(s->id)
				});

			jobs.emplace_back(j);
			
		}

		_jobs.run(jobs);
		_jobs.wait(on_create_parent);

		const auto systems = _game.get_systems();

		//call on_connect for new entities
		const auto on_connect_parent = _jobs.create();
		jobs.clear();
		for (const auto s : systems)
		{
			const auto ents = get_added_entites(s.attached_entities, _prev_frame, t);
			auto& sys_data = _game.get_system_data(s.system->id);
			 
			for (const auto e : ents)
			{
				const auto j = _jobs.create_child(on_connect_parent, s.system->on_connect,
					render_job_data{ e, &_game, m, t, i, sys_data });

				jobs.emplace_back(j);
			}
		}

		_jobs.run(jobs);
		_jobs.wait(on_connect_parent);

		//call on_disconnect for removed entities
		const auto on_disconnect_parent = _jobs.create();
		jobs.clear();
		for (const auto s : systems)
		{
			const auto ents = get_removed_entites(s.attached_entities, _prev_frame, t);
			auto& sys_data = _game.get_system_data(s.system->id);

			for (const auto e : ents)
			{
				const auto j = _jobs.create_child(on_disconnect_parent, s.system->on_disconnect,
					render_job_data{ e, &_game, m, t, i, sys_data });

				jobs.emplace_back(j);
			}
		}

		_jobs.run(jobs);
		_jobs.wait(on_disconnect_parent);

		//call on_tick for systems
		const auto on_tick_parent = _jobs.create();
		jobs.clear();
		for (auto& s : systems)
		{
			const auto entities_curve = s.attached_entities.get();
			const auto ents = entities_curve.get(t);

			auto& sys_data = _game.get_system_data(s.system->id);

			for (const auto e : ents)
			{
				const auto j = _jobs.create_child(on_tick_parent, s.system->tick,
					render_job_data{ e, &_game, m, t, i, sys_data });

				jobs.emplace_back(j);
			}
		}

		_jobs.run(jobs);
		_jobs.wait(on_tick_parent);
		_jobs.clear();

		_game.clear_new_systems();
		_prev_frame = t;
	}
}
