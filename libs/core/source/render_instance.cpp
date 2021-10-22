#include "hades/render_instance.hpp"

#include <type_traits>

#include "hades/core_curves.hpp"
#include "hades/render_interface.hpp"

namespace hades 
{
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
		const auto dead_ents = _interface->get_removed_objects();
		for (const auto o : dead_ents)
			_extra.systems.detach_all({ o });

		auto constant_job_data = render_job_data{ t, &_extra, &_extra.systems };
		constant_job_data.level_data = _interface;
		constant_job_data.render_output = &i;
		//constant_job_data.mission = nullptr;
		const auto next = update_level<const common_interface>(constant_job_data,
			*_interface);

		//erase dead ents
		for (auto o : dead_ents)
		{
			auto ref = object_ref{ o };
			auto obj = hades::state_api::get_object_ptr(ref, _extra);
			if(obj)
				hades::state_api::erase_object(*obj, _interface->get_state(), _extra);
		}

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
