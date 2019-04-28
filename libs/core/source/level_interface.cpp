#include "Hades/level_interface.hpp"

#include "hades/data.hpp"
#include "hades/level.hpp"
#include "hades/game_system.hpp"

namespace hades 
{
	common_implementation_base::common_implementation_base(const level_save& sv) 
		: _entity_names{ sv.names },
		_next{ static_cast<entity_id::value_type>(sv.next_id) }, _curves(sv.curves)
	{}

	entity_id common_implementation_base::create_entity()
	{
		return entity_id{ ++_next };
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
		assert(entity < entity_id{ _next });

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

	curve_data& common_implementation_base::get_curves()
	{
		return _curves;
	}

	const curve_data& common_implementation_base::get_curves() const
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
	}

	render_implementation::render_implementation()
		: common_implementation{ level_save{} }
	{}
}