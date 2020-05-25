#include "Hades/level_interface.hpp"

#include "hades/core_curves.hpp"
#include "hades/data.hpp"
#include "hades/level.hpp"
#include "hades/game_system.hpp"
#include "hades/objects.hpp"

namespace hades 
{
	common_implementation_base::common_implementation_base(const level_save& sv) 
		: _entity_names{ sv.objects.names }, 
		_next{ sv.objects.next_id }, _curves{ sv.objects.curves },
		_size{ static_cast<world_unit_t>(sv.source.map_x), static_cast<world_unit_t>(sv.source.map_y) }
	{
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

	entity_id common_implementation_base::create_entity()
	{
		return increment(_next);
	}

	entity_id common_implementation_base::create_entity(const object_instance &o,
		time_point t)
	{
		const auto id = o.id == bad_entity ? create_entity() : o.id;
		
		if(!std::empty(o.name_id))
			name_entity(id, o.name_id, t);

		//add all the curves from o to the entity 'id'
		auto& curves = get_curves();
		
		for (const auto& [c, v] : get_all_curves(o))
		{
			assert(c);
			assert(!v.valueless_by_exception());
			const auto index = curve_index_t{ id, c->id };

			std::visit([&, cur = c](auto&& v)->void {
				using T = std::decay_t<decltype(v)>;				
				auto &curve_map = get_curve_list<T>(curves);
				if (curve_map.find(index) == std::end(curve_map))
				{
					auto new_curve = game_property<T>{ t, std::move(v) };
					curve_map.emplace(index, std::move(new_curve));
				}

				return;
				}, v);
		}

		//add object type as a curve
		const auto obj_type_c = get_object_type_curve();
		assert(obj_type_c);

		using ObjType = resources::curve_types::unique;
		auto obj_type = game_property<ObjType>{ t, o.obj_type ? o.obj_type->id : unique_zero };
		auto &obj_type_curves = get_curve_list<ObjType>(curves);
		obj_type_curves.emplace(curve_index_t{ id, obj_type_c->id }, std::move(obj_type));

		//attach systems
		if (o.obj_type)
		{
			const auto systems = get_systems(*o.obj_type);
			for (const auto s : systems)
				attach_system(id, s->id, t);
		}

		return id;
	}

	entity_id common_implementation_base::get_entity_id(std::string_view name, time_point) const
	{
		if (const auto ent_name = _entity_names.find(to_string(name)); ent_name != std::end(_entity_names))
			return ent_name->second;

		return bad_entity;
	}

	void common_implementation_base::name_entity(entity_id entity, std::string_view name, time_point)
	{
		assert(entity < next(_next));
		_entity_names[to_string(name)] = entity;
		return;
	}

	curve_data& common_implementation_base::get_curves() noexcept
	{
		return _curves;
	}

	const curve_data& common_implementation_base::get_curves() const noexcept
	{
		return _curves;
	}

	const terrain_map& common_implementation_base::get_world_terrain() const noexcept
	{
		return _terrain;
	}

	world_rect_t common_implementation_base::get_world_bounds() const noexcept
	{
		return world_rect_t{ world_vector_t{}, _size };
	}

	std::any &common_implementation_base::get_level_local_ref(unique_id u)
	{
		return _level_local[u];
	}

	static const resources::player_input* get_if_player_input(const level_save& sv)
	{
		const auto id = sv.source.player_input_script;
		if (id == unique_id::zero)
			return nullptr;

		return data::get<resources::player_input>(sv.source.player_input_script);
	}

	game_implementation::game_implementation(const level_save &sv) 
		: common_implementation_base{sv}, _player_input{get_if_player_input(sv)}
	{
		// NOTE: this is checked on release when reading savefiles 
		//       and converting levels into saves
		assert(sv.objects.systems.size() == sv.objects.systems_attached.size());

		//convert from curve<vector<entity_id>>
		// to curve<vector<pair<entity_id, timePoint>>>
		for (std::size_t i = 0; i < sv.objects.systems.size(); ++i)
		{
			auto attached = name_list{};
			attached.reserve(size(sv.objects.systems_attached[i]));
			for (const auto& a : sv.objects.systems_attached[i])
				attached.emplace_back(a, time_point{});

			_systems.set_attached(sv.objects.systems[i]->id, std::move(attached));
		}
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
