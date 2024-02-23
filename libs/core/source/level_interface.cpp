#include "hades/level_interface.hpp"

#include "hades/core_curves.hpp"
#include "hades/data.hpp"
#include "hades/level.hpp"
#include "hades/level_scripts.hpp"
#include "hades/game_system.hpp"
#include "hades/objects.hpp"
#include "hades/save_load_api.hpp"

namespace hades 
{
	object_ref game_implementation::create_object(const object_instance &o, time_point t)
	{
		auto obj = state_api::make_object(o, t, _state, _extras);
		_new_objects.emplace_back(*obj.ptr);
		return obj;
	}

	object_ref game_implementation::clone_object(object_ref o, time_point t)
	{
		const auto& current_obj = state_api::get_object(o, _extras);
		auto obj = state_api::clone_object(current_obj, t, _state, _extras);
		_new_objects.emplace_back(*obj.ptr);
		return obj;
	}

	void game_implementation::destroy_object(object_ref o, time_point t)
	{
		auto obj = state_api::get_object_ptr(o, _extras);
		if (!obj) return;

		state_api::destroy_object(o, t, _state, _extras);
		_destroy_objects.emplace_back(obj);

		// if this object is still in the new_objects list(client hasn't been told about it)
		// then just erase it from the new list, the client will never know of this obj
		if (auto iter = std::ranges::find(_new_objects, o.id, &game_obj::id);
			iter != end(_new_objects))
		{
			_new_objects.erase(iter);
		}
		else
			_removed_objects.emplace_back(o.id);
	}

	std::vector<game_obj*> game_implementation::get_destroyed_objects() noexcept
	{
		return std::exchange(_destroy_objects, {});
	}

	std::vector<game_obj> game_implementation::get_new_objects() noexcept
	{
		return std::exchange(_new_objects, {});
	}

	std::vector<entity_id> game_implementation::get_removed_objects() noexcept
	{
		return std::exchange(_removed_objects, {});
	}

	void game_implementation::name_object(std::string_view s, object_ref o, time_point t)
	{
		state_api::name_object(string{ s }, o, t, _state);
		return;
	}

	static const resources::player_input* get_if_player_input(const level_save& sv)
	{
		const auto id = sv.source.player_input_script;
		if (id == unique_id::zero)
			return nullptr;

		return data::get<resources::player_input>(sv.source.player_input_script);
	}

	game_implementation::game_implementation(const level_save& sv,
		game_interface* mission, const std::vector<player_data>* players)
        : _size{ static_cast<world_unit_t>(sv.source.map_x),
        static_cast<world_unit_t>(sv.source.map_y) },
          _player_input{ get_if_player_input(sv) }
	{
		if (!std::empty(sv.source.terrain.tile_layer.tiles))
		{
			_terrain = to_terrain_map(sv.source.terrain);
		}

		_state.next_id = sv.objects.next_id;

		const auto load_script_id = sv.source.on_load;
		
		{
			auto game_data = system_job_data{};
			game_data.current_time = sv.level_time;
			game_data.extra = &_extras;
			game_data.systems = &_extras.systems;
			game_data.level_data = this;
			game_data.mission_data = mission;
			game_data.players = players;

			detail::set_data(&game_data);

			if (load_script_id)
			{
				const auto on_load = data::get<resources::load_script>(load_script_id);
				std::invoke(on_load->function, sv);
			}
			else
			{
				game::level::load(sv);
			}
		}

		_new_objects.reserve(_extras.objects.size());
		for (auto& o : _extras.objects)
			_new_objects.emplace_back(o);

		return;
	}

	game_implementation::~game_implementation()
	{
        // TODO: constructor for system_job_data, and all it's uses
		auto data = system_job_data{ time_point{}, &_extras, &_extras.systems };
		data.level_data = this;

		auto& sys_behaviours = _extras.systems;
		// call on_destroy for all systems
		for (auto s : sys_behaviours.get_systems())
		{
			if (s->system->on_destroy)
			{
				auto& sys_data = sys_behaviours.get_system_data(s->system->id);
				auto game_data = data;
				game_data.entity = activated_object_view{ sys_behaviours.get_entities(*s), time_point::max() };
				game_data.system = s->system->id;
				game_data.system_data = &sys_data;

				detail::set_data(&game_data);
				std::invoke(s->system->on_destroy);
			}
		}
		return;
	}

	object_ref game_implementation::get_object_ref(std::string_view s, time_point t) noexcept
	{
		return state_api::get_object_ref(s, t, _state, _extras);
	}

	void game_implementation::update_input_queue(unique_id p, std::vector<action> input, time_point)
	{
		//TODO: FIXME
		//_input_history[p].insert(t, std::move(input)); //DEBUG: player input log, in actions, rather than keys
		auto& q = _input_queue[p];
		if (empty(q))
			q = std::move(input);
		else // dont erase unused input
			q.insert((end(q)), begin(input), end(input));

		return;
	}

	game_implementation::input_queue_t game_implementation::get_and_clear_input_queue() noexcept
	{
		return std::exchange(_input_queue, {});
	}
}
