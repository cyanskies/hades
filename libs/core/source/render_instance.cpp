#include "hades/render_instance.hpp"

#include <type_traits>

#include "hades/core_curves.hpp"
#include "hades/render_interface.hpp"

namespace hades 
{
	static void setup_systems_for_new_object(entity_id e, unique_id obj_type,
		time_point t, system_behaviours<render_system>& sys)
	{
		using namespace std::string_literals;

		//TODO: implement to_string for timer types
		try
		{
			const auto o = data::get<resources::object>(obj_type);

			const auto systems = get_render_systems(*o);

			for (const auto s : systems)
				sys.attach_system(e, s->id, t);
			return;
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
	}

	static void activate_ents(const client_interface* i,
		system_behaviours<render_system>& systems, std::unordered_set<entity_id>& activated,
		time_point time)
	{
		assert(i);
		const auto &curves = i->get_curves();
		const auto obj_type_var = get_object_type_curve_id();
		//unique curves contain the object types for entities
		for (const auto& [key, val] : curves.unique_curves)
		{
			if (key.second != obj_type_var)
				continue;

			if (activated.find(key.first) != std::end(activated))
				continue;

			assert(!val.empty());
			const auto [t, o] = *val.begin();

			//TODO: can we use the time from the curve,
			//		might be possible after syncing the 
			//		server and client time
			setup_systems_for_new_object(key.first, o, time, systems);

			activated.emplace(key.first);
		}
		return;
	}

	static void deactivate_ents(const client_interface* i,
		system_behaviours<render_system>& systems, std::unordered_set<entity_id>& activated,
		time_point time)
	{
		assert(i);
		const auto &curves = i->get_curves();
		const auto alive_id = get_alive_curve_id();
		for (const auto& [key, val] : curves.bool_curves)
		{
			if (key.second != alive_id)
				continue;

			const auto activated_iter = activated.find(key.first);
			if (activated_iter == std::end(activated))
				continue;

			const auto alive = val.get(time);

			if (!alive)
			{
				activated.erase(activated_iter);
				systems.detach_all(key.first, time);
			}
		}
		return;
	}

	render_instance::render_instance(common_interface* i) : _interface{i}
	{
		if(i)
			activate_ents(i, _systems, _activated_ents, _current_frame);
		return;
	}

	void render_instance::make_frame_at(time_point t, const common_interface *m, render_interface &i)
	{
		assert(_interface);

		//check for new entities and setup systems
		activate_ents(_interface, _systems, _activated_ents, _current_frame);
		deactivate_ents(_interface, _systems, _activated_ents, _current_frame);

		//assert(m);
		const auto dt = time_duration{ t - _current_frame };

		auto make_render_job_data = [m, &i](unique_id sys, std::vector<entity_id> e, common_interface* g, system_behaviours<render_system> *s, time_point prev,
			time_duration dt, system_data_t* d)->render_job_data {
				return render_job_data{sys, std::move(e), g, s, prev + dt, &i, d };
		};

		const auto next = update_level(_prev_frame, _current_frame, dt,
			*_interface, _systems, make_render_job_data);

		i.prepare(); //copy sprites into vertex buffer

		_prev_frame = _current_frame;
		_current_frame = next;

		assert(_current_frame == t);
	}
}
