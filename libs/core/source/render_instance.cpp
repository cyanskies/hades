#include "hades/render_instance.hpp"

#include <type_traits>

#include "hades/core_curves.hpp"
#include "hades/render_interface.hpp"

namespace hades 
{
	struct empty_struct_t {};

	constexpr static auto empty_struct = empty_struct_t{};

	template<typename T, typename Callback = empty_struct_t>
	static void merge_input(const std::vector<exported_curves::export_set<T>> & input, std::size_t size, curve_data & output, time_point time = time_point{}, Callback callback = empty_struct)
	{
		//TODO: use size
		auto& output_curves = get_curve_list<T>(output);

		auto index = std::size_t();
		while(index < size)// (const auto& [ent, var, frames] : input)
		{
			const auto& [ent, var, frames] = input[index];
			const auto id = std::decay_t<decltype(output_curves)>::key_type{ ent, var };
			const auto exists = output_curves.exists_no_async(id);

			if (exists)
			{
				auto& c = output_curves.get_no_async(id);

				//TODO: bulk insertion for curves
				for (auto& f : frames)
					c.set(f.first, f.second);
			}
			else
			{
				//TODO: cache these somewhere local, only need the c_types
				const auto c = data::get<resources::curve>(id.second);
				auto new_c = curve<T>{ c->c_type };

				for (auto& f : frames)
					new_c.set(f.first, f.second);

				output_curves.create(id, std::move(new_c));

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

						//TODO: if we set the time point based on this then,
						// we often skip on_connect, since it will need to,
						// have happened in the past, we need to pass _lastFrame
						// so that we connect after we find out about it,
						// not when the object was origionally created on the server

						// the time_point of obj-type should be 
						// the creation time of the entity
						const auto type = frames[0];

						//use callback to request object setup
						std::invoke(callback, ent, type.second, time);
					}
				}
			}

			++index;
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

	render_instance::render_instance() 
		: _jobs{*console::get_int(cvars::server_threadcount, cvars::default_value::server_threadcount)}
	{
	}

	void render_instance::input_updates(const exported_curves& input)
	{
		auto& curves = _game.get_curves();

		merge_input(input.int_curves, input.sizes[0], curves);
		merge_input(input.float_curves, input.sizes[1], curves);
		merge_input(input.bool_curves, input.sizes[2], curves);
		merge_input(input.string_curves, input.sizes[3], curves);
		merge_input(input.object_ref_curves, input.sizes[4], curves);

		auto obj_type_callback = [game = &_game](entity_id e, unique_id u, time_point t){
			setup_systems_for_new_object(e, u, t, *game);
		};

		merge_input(input.unique_curves, input.sizes[5], curves, _current_frame, obj_type_callback);

		merge_input(input.int_vector_curves, input.sizes[6], curves);
		merge_input(input.float_vector_curves, input.sizes[7], curves);
		merge_input(input.object_ref_vector_curves, input.sizes[8], curves);
		merge_input(input.unique_vector_curves, input.sizes[9], curves);
	}

	void render_instance::make_frame_at(time_point t, render_implementation *m, render_interface &i)
	{
		const auto dt = time_duration{ t - _current_frame };

		auto make_render_job_data = [m, &i](entity_id e, game_interface* g, time_point prev,
			time_duration dt, system_data_t* d)->render_job_data {
				return render_job_data{ e, g, m, prev + dt, &i, d };
		};

		//TODO: retire multithreaded rendering for good
		// seriously, 80% of time stuck in deadlock avoidance
		static const auto server_threads = console::get_int(cvars::render_threadcount);
		const auto desired_threads = server_threads->load();
		const auto threads = detail::get_update_thread_count(desired_threads);
		bool async = threads > 1;
		i.set_async(async);

		const auto next = update_level(_jobs, _prev_frame, _current_frame, dt,
			_game, desired_threads, make_render_job_data);

		_prev_frame = _current_frame;
		_current_frame = next;

		assert(_current_frame == t);
	}
}
