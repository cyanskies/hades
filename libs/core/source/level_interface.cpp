#include "Hades/level_interface.hpp"

#include "hades/core_curves.hpp"
#include "hades/data.hpp"
#include "hades/level.hpp"
#include "hades/game_system.hpp"
#include "hades/objects.hpp"

namespace hades 
{
	object_ref game_implementation::create_object(const object_instance &o)
	{
		return state_api::make_object(o, _state, _extras);
	}

	void game_implementation::name_object(std::string_view s, object_ref o)
	{
		state_api::name_object(string{ s }, o, _state);
		return;
	}

	static const resources::player_input* get_if_player_input(const level_save& sv)
	{
		const auto id = sv.source.player_input_script;
		if (id == unique_id::zero)
			return nullptr;

		return data::get<resources::player_input>(sv.source.player_input_script);
	}

	static std::tuple<game_state, extra_state> fill_state(const level_save &sv)
	{
		auto s = game_state{};
		auto e = extra_state{};
		s.next_id = sv.objects.next_id;

		for (const auto o : sv.objects.objects)
			state_api::make_object(o, s, e);

		return { s, e };
	}

	game_implementation::game_implementation(const level_save& sv)
		: _player_input{ get_if_player_input(sv) },
		_size{ static_cast<world_unit_t>(sv.source.map_x), static_cast<world_unit_t>(sv.source.map_y) }
	{
		std::tie(_state, _extras) = fill_state(sv);

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

	object_ref game_implementation::get_object_ref(std::string_view s) noexcept
	{
		return state_api::get_object_ref(s, _state, _extras);
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
