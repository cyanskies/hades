#include "Hades/level_interface.hpp"

#include "hades/core_curves.hpp"
#include "hades/data.hpp"
#include "hades/level.hpp"
#include "hades/game_system.hpp"
#include "hades/objects.hpp"

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

	void game_implementation::destroy_object(object_ref o)
	{
		//TODO: must account for current time
		auto obj = state_api::get_object_ptr(o, _extras);
		if (!obj) return;

		state_api::destroy_object(o, _extras);
		_destroy_objects.emplace_back(obj);
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

	static std::tuple<game_state, extra_state<game_system>> fill_state(const level_save &sv)
	{
		auto s = game_state{};
		auto e = extra_state<game_system>{};
		s.next_id = sv.objects.next_id;

		for (const auto &o : sv.objects.objects)
			state_api::loading::restore_object(o, s, e);

		return { std::move(s), std::move(e) };
	}

	game_implementation::game_implementation(const level_save& sv)
		: _player_input{ get_if_player_input(sv) },
		_size{ static_cast<world_unit_t>(sv.source.map_x), static_cast<world_unit_t>(sv.source.map_y) }
	{
		std::tie(_state, _extras) = fill_state(sv);

		_new_objects.reserve(_extras.objects.size());
		for (auto& o : _extras.objects)
			_new_objects.emplace_back(o);
		
		if (!std::empty(sv.source.tile_map_layer.tiles))
		{
			const auto raw_map = raw_terrain_map{
				sv.source.terrainset,
				sv.source.terrain_vertex,
				sv.source.terrain_layers,
				sv.source.tile_map_layer
			};

			_terrain = to_terrain_map(raw_map);
		}
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
