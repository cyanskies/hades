#include "Hades/level_interface.hpp"

#include "hades/core_curves.hpp"
#include "hades/data.hpp"
#include "hades/level.hpp"
#include "hades/game_system.hpp"
#include "hades/objects.hpp"

namespace hades 
{
	common_implementation_base::common_implementation_base(const level_save& sv) 
		: _entity_names{ sv.names }, 
		_next{ static_cast<entity_id::value_type>(sv.next_id) }, _curves{ sv.curves },
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
		return entity_id{ ++_next };
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

			std::visit([&](auto&& v)->void {
				using T = std::decay_t<decltype(v)>;				
				auto &curve_map = get_curve_list<T>(curves);
				if (curve_map.find(index) == std::end(curve_map))
				{
					auto new_curve = curve<T>{ c->c_type };
					new_curve.set(t, std::move_if_noexcept(v));
					curve_map.emplace(index, std::move(new_curve));
				}

				return;
				}, v);
		}

		//add object type as a curve
		assert(o.obj_type);
		const auto obj_type_c = get_object_type_curve();
		assert(obj_type_c);

		using ObjType = resources::curve_types::unique;
		auto obj_type = curve<ObjType>{ obj_type_c->c_type };
		obj_type.set(t, o.obj_type->id);

		auto &obj_type_curves = get_curve_list<ObjType>(curves);
		obj_type_curves.emplace(curve_index_t{ id, obj_type_c->id }, std::move(obj_type));

		//attach systems
		const auto systems = get_systems(*o.obj_type);
		for (const auto s : systems)
			attach_system(id, s->id, t);

		return id;
	}

	entity_id common_implementation_base::get_entity_id(std::string_view name, time_point t) const
	{
		const auto name_map = _entity_names.get(t);
		if (const auto ent_name = name_map.find(to_string(name)); ent_name != std::end(name_map))
			return ent_name->second;

		return bad_entity;
	}

	void common_implementation_base::name_entity(entity_id entity, std::string_view name, time_point time)
	{
		assert(entity < entity_id{ _next + 1 });

		auto name_map = _entity_names.get(time);
		name_map[to_string(name)] = entity;
		_entity_names.insert(time, name_map);
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
		: common_implementation_base{sv}, _save{sv}, _player_input{get_if_player_input(sv)}
	{
		// NOTE: this is checked on release when reading savefiles 
		//       and converting levels into saves
		assert(sv.systems.size() == sv.systems_attached.size());

		//convert from curve<vector<entity_id>>
		// to curve<vector<pair<entity_id, timePoint>>>
		for (std::size_t i = 0; i < sv.systems.size(); ++i)
		{
			auto attached = name_list{};
			for (const auto& a : sv.systems_attached[i])
			{
				auto ents = std::vector<attached_ent>{};
				ents.reserve(std::size(a.second));
				for (auto e : a.second)
					ents.emplace_back(e, time_point{});

				attached.set(a.first, std::move(ents));
			}

			_systems.set_attached(sv.systems[i]->id, std::move(attached));
		}
	}

	void game_implementation::update_input_queue(std::vector<server_action> input, time_point t)
	{
		_input_history.emplace_back(input, t); //DEBUG: player input log, in actions, rather than keys
		_input_queue = std::move(input);
		return;
	}

	std::vector<server_action> game_implementation::get_and_clear_input_queue() noexcept
	{
		return std::exchange(_input_queue, {});
	}

	render_implementation::render_implementation()
		: common_implementation_base{ level_save{} }
	{}
}