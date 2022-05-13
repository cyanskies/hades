#ifndef HADES_GAME_SYSTEM_RESOURCES_HPP
#define HADES_GAME_SYSTEM_RESOURCES_HPP

#include <vector>

#include "hades/curve_types.hpp"
#include "hades/exceptions.hpp"

namespace hades::data
{
	class data_manager;
}

namespace hades
{
	// TODO: get rid of this
	//	creates the simple renderer system
	void register_game_system_resources(data::data_manager&);

	class system_error : public runtime_error
	{
	public:
		using runtime_error::runtime_error;
	};

	// attached ent, a pair of object-ref and the time after which they are allowed to update
	struct object_time {
		object_ref object;
		time_point next_activation;
	};

	inline bool operator==(const object_time& l, const object_time& r) noexcept
	{
		return l.object == r.object;
	}

	inline bool operator!=(const object_time& l, const object_time& r) noexcept
	{
		return !(l == r);
	}

	//using attached_ent = std::pair<object_ref, time_point>;
	using name_list = std::vector<object_time>; // TODO: rename

}

namespace hades::resources
{
	struct system_t
	{};

	//a system stores a job function
	struct system : public resource_type<system_t>
	{
		using system_func = std::function<void()>;

		system_func on_create, //called on system creation
			on_connect,			//called when attached to an entity
			on_disconnect,     //called when detatched from ent
			tick,				//called every tick
			on_destroy;			//called on system destruction: mission and player info is not available
		//	on_event?

		//if loaded from a manifest then it should be loaded from scripts
		//if it's provided by the application, then source is empty, and no laoder function is provided.
	};

	struct render_system_t
	{};

	struct render_system : public resource_type<render_system_t>
	{
		using system_func = std::function<void()>;

		system_func on_create,
			on_connect,
			on_disconnect,
			tick,
			on_destroy;
	};
}

namespace hades::detail
{
	template<typename SystemType>
	struct basic_system
	{
		using system_t = SystemType;

		//this holds the systems, name and id, and the function that the system uses.
		const system_t* system = nullptr;
		//list of entities attached to this system, over time
		name_list attached_entities;
		name_list new_ents;
		name_list created_ents;
		name_list removed_ents;
	};
}

#endif //!HADES_GAME_SYSTEM_RESOURCES_HPP