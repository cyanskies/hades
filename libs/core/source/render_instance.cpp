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

    void render_instance::make_frame_at(time_point t, const common_interface *, render_interface &i)
	{
		assert(_interface);

		_create_new_objects(_interface->get_new_objects());
		
		//detach dead entities
		const auto dead_ents = _interface->get_removed_objects();
		for (const auto o : dead_ents)
		{
			auto ref = object_ref{ o };
			_extra.systems.detach_all(ref);
			// we have to erase our game_obj at the same time the server does
			// so that a local server wont end up with
			// the render game_obj holding ptrs to the erased server data
			auto obj = state_api::get_object_ptr(ref, _extra);
			if (obj)
			{
				obj->id = bad_entity;
				_extra.objects.erase(obj);
			}
		}

        // TODO: make rjd constructor for all usages
		auto constant_job_data = render_job_data{ t, &_extra, &_extra.systems };
		constant_job_data.level_data = _interface;
		constant_job_data.render_output = &i;
		//constant_job_data.mission = nullptr;
		const auto next = update_level<const common_interface>(constant_job_data,
			*_interface);

		//erase dead ents
		// TODO: erase dead objects data(probably in remote_level)
		//	doing it here resulted in double erase
		//	maybr just check if they're still alive before erasing them
	

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
            for (const auto& s : resources::object_functions::get_render_systems(*o->object_type))
				_extra.systems.attach_system(object_ref{ o->id, &*o }, s.id());
		}
	}
}
