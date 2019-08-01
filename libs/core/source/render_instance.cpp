#include "hades/render_instance.hpp"

#include <type_traits>

#include "hades/core_curves.hpp"

namespace hades 
{
	struct empty_struct_t {};

	constexpr static auto empty_struct = empty_struct_t{};

	template<typename T, typename Callback = empty_struct_t>
	static void merge_input(const std::vector<exported_curves::export_set<T>> & input, curve_data & output, time_point time = time_point{}, Callback callback = empty_struct)
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

		merge_input(input.unique_curves, curves, _current_frame, obj_type_callback);

		merge_input(input.int_vector_curves, curves);
		merge_input(input.float_vector_curves, curves);
		merge_input(input.object_ref_vector_curves, curves);
		merge_input(input.unique_vector_curves, curves);
	}

	void render_instance::make_frame_at(time_point t, render_implementation *m, render_interface &i)
	{
		const auto dt = time_duration{ t - _current_frame };

		auto make_render_job_data = [m, &i](entity_id e, game_interface* g, time_point prev,
			time_duration dt, system_data_t* d)->render_job_data {
				return render_job_data{ e, g, m, prev + dt, &i, d };
		};

		const auto next = update_level(_jobs, _prev_frame, _current_frame, dt,
			_game, make_render_job_data);

		_prev_frame = _current_frame;
		_current_frame = next;

		assert(_current_frame == t);
	}
}
