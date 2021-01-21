#include "hades/render_instance.hpp"

#include <type_traits>

#include "hades/core_curves.hpp"
#include "hades/render_interface.hpp"

namespace hades 
{
	static auto make_render_job_data_func(render_interface* i) noexcept
	{
		return [i](render_job_data d, const common_interface* g, const common_interface* m, time_duration, const std::vector<player_data>*)noexcept->render_job_data {
			d.level_data = g;
			d.render_output = i;
			return d;
		};
	}

	render_instance::render_instance(common_interface* i) : _interface{i}
	{
		assert(i);
		return;
	}

	extra_state<render_system>& render_instance::get_extras() noexcept
	{
		return _extra;
	}

	void render_instance::make_frame_at(time_point t, const common_interface *m, render_interface &i)
	{
		assert(_interface);

		_create_new_objects(_interface->get_new_objects());
		
		//detach dead entities
		for (const auto o : _interface->get_removed_objects())
			_extra.systems.detach_all({ o });

		const auto dt = time_duration{ t - _prev_frame };
		auto make_render_job_data = make_render_job_data_func(&i);
		auto constant_job_data = render_job_data{ t, &_extra, &_extra.systems };
		const auto next = update_level<const common_interface>(constant_job_data, dt,
			*_interface, m, nullptr, make_render_job_data);

		i.prepare(); //copy sprites into vertex buffer

		_prev_frame = t;
	}

	void render_instance::_create_new_objects(std::vector<game_obj> objects)
	{
		//update extra with new object information
		auto& obj_list = _extra.objects;

		std::vector<game_obj*> new_objects;
		new_objects.reserve(size(objects));
		for (auto& o : objects)
			new_objects.emplace_back(obj_list.insert(std::move(o)));

		for (auto& o : new_objects)
		{
			const auto id = o->id;
			const auto systems = get_render_systems(*o->object_type);
			for (const auto s : systems)
				_extra.systems.attach_system(object_ref{ o->id, &*o }, s->id);
		}
	}
}
