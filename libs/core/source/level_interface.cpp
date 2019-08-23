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
		_next{ static_cast<entity_id::value_type>(sv.next_id) }, _curves{ sv.curves }
	{}

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
		//FIXME: get_all_curves is returning curves that are infected with std::monostate.
		for (const auto& [c, v] : get_all_curves(o))
		{
			assert(c);
			assert(!v.valueless_by_exception());
			const auto index = curve_index_t{ id, c->id };

			std::visit([&](auto&& v)->void {
				using T = std::decay_t<decltype(v)>;				
				auto &curve_map = get_curve_list<T>(curves);
				if (!curve_map.exists(index))
				{
					auto new_curve = curve<T>{ c->c_type };
					new_curve.set(t, std::move_if_noexcept(v));
					curve_map.create(index, std::move(new_curve));
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
		obj_type_curves.create(curve_index_t{ id, obj_type_c->id }, std::move(obj_type));

		//attach systems
		const auto systems = get_systems(*o.obj_type);
		for (const auto s : systems)
			attach_system(id, s->id, t);

		return id;
	}

	entity_id common_implementation_base::get_entity_id(std::string_view name, time_point t) const
	{
		const auto names = _entity_names.get();
		const auto name_map = names.get(t);
		if (const auto ent_name = name_map.find(to_string(name)); ent_name != std::end(name_map))
			return ent_name->second;

		return bad_entity;
	}

	void common_implementation_base::name_entity(entity_id entity, std::string_view name, time_point time)
	{
		assert(entity < entity_id{ _next + 1 });

		name_curve_t ent_names{ curve_type::error }, updated_names{ curve_type::error };
		do
		{
			ent_names = _entity_names.get();
			updated_names = ent_names;
			//get the closest list of names to the current time
			auto name_map = updated_names.get(time);

			//if entities can be renamed
			name_map[to_string(name)] = entity;

			//if entities can not be renamed
			//if (name_map.find(name) != std::end(name_map))
			//	name_map[name] = entity;
			//else
			//	;//throw?

			//insert the new name map back into the curve
			updated_names.insert(time, name_map);

			//replace the shared name list with the updated one
		} while (!_entity_names.compare_exchange(ent_names, updated_names));
	}

	void common_implementation_base::add_input(input_system::action_set a, time_point t)
	{
		_input.insert(t, std::move(a));
	}

	curve_data& common_implementation_base::get_curves() noexcept
	{
		return _curves;
	}

	const curve_data& common_implementation_base::get_curves() const noexcept
	{
		return _curves;
	}

	game_implementation::game_implementation(const level_save &sv) 
		: common_implementation{sv}
	{
		// NOTE: this is checked on release when reading savefiles 
		//       and converting levels into saves
		assert(sv.systems.size() == sv.systems_attached.size());

		//TODO: check that systems have been loaded by now
		std::vector<game_system> systems;
		auto attach_list = sv.systems_attached;

		for (std::size_t i = 0; i < sv.systems.size(); ++i)
			systems.emplace_back(sv.systems[i], std::move(attach_list[i]));

		_systems = std::move(systems);

		//create the special world entity
		//set the world size from the level source
		//creates the world entity if it didn't already exist
		auto world_ent = get_entity_id(world_entity_name, time_point{});

		if (world_ent == bad_entity)
		{
			world_ent = create_entity();
			name_entity(world_ent, world_entity_name, time_point{});
		}

		const auto [size_x, size_y] = get_size_curve_id();
		auto& float_curves = get_curve_list<resources::curve_types::float_t>(get_curves());
		const auto x_id = curve_index_t{ world_ent, size_x },
			y_id = curve_index_t{ world_ent, size_y };

		//create the world size if it hasn't already been done 
		if (!float_curves.exists_no_async(x_id))
		{
			curve<resources::curve_types::float_t> s_x{ curve_type::const_c };
			s_x.set(time_point{}, static_cast<resources::curve_types::float_t>(sv.source.map_x));
			float_curves.create(x_id, std::move(s_x));
		}

		if (!float_curves.exists_no_async(y_id))
		{
			curve<resources::curve_types::float_t> s_y{ curve_type::const_c };
			s_y.set(time_point{}, static_cast<resources::curve_types::float_t>(sv.source.map_y));
			float_curves.create(y_id, std::move(s_y));
		}
	}

	render_implementation::render_implementation()
		: common_implementation{ level_save{} }
	{}
}