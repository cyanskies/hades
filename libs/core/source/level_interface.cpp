#include "Hades/level_interface.hpp"

#include "hades/data.hpp"
#include "hades/level.hpp"
#include "hades/game_system.hpp"

namespace hades 
{
	template<>
	curve_data::curve_map<resources::curve_types::int_t> &get_curve_list(curve_data &data)
	{
		return data.int_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::float_t> &get_curve_list(curve_data &data)
	{
		return data.float_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::bool_t> &get_curve_list(curve_data &data)
	{
		return data.bool_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::string> &get_curve_list(curve_data &data)
	{
		return data.string_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::object_ref> &get_curve_list(curve_data &data)
	{
		return data.object_ref_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::unique> &get_curve_list(curve_data &data)
	{
		return data.unique_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::vector_int> &get_curve_list(curve_data &data)
	{
		return data.int_vector_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::vector_float> &get_curve_list(curve_data &data)
	{
		return data.float_vector_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::vector_object_ref> &get_curve_list(curve_data &data)
	{
		return data.object_ref_vector_curves;
	}

	template<>
	curve_data::curve_map<resources::curve_types::vector_unique> &get_curve_list(curve_data &data)
	{
		return data.unique_vector_curves;
	}

	GameInterface::GameInterface(const level_save &sv) : _entity_names(sv.names),
		_next(static_cast<entity_id::value_type>(sv.next_id)), _curves(sv.curves)
	{
		// NOTE: this is checked on release when reading savefiles 
		//       and converting levels into saves
		assert(sv.systems.size() == sv.systems_attached.size()); 

		//TODO: check that systems have been loaded by now
		std::vector<GameSystem> systems;
		auto attach_list = sv.systems_attached;

		for (std::size_t i = 0; i < sv.systems.size(); ++i)
			systems.emplace_back(sv.systems[i], std::move(attach_list[i]));

		_systems = std::move(systems);
	}

	entity_id GameInterface::createEntity()
	{
		return entity_id{ ++_next };
	}

	entity_id GameInterface::getEntityId(const types::string &name, time_point t) const
	{
		const auto names = _entity_names.get();
		const auto name_map = names.get(t);
		if (const auto ent_name = name_map.find(name); ent_name != std::end(name_map))
			return ent_name->second;

		return bad_entity;
	}

	curve_data &GameInterface::getCurves()
	{
		return _curves;
	}

	const curve_data &GameInterface::getCurves() const
	{
		return _curves;
	}

	void GameInterface::attachSystem(entity_id entity, unique_id sys, time_point t)
	{
		const auto lock = std::lock_guard{ _system_list_mut };

		auto &system = FindSystem(sys);
		auto ents = system.attached_entities.get();
		auto ent_list = ents.get(t);
		auto found = std::find(ent_list.begin(), ent_list.end(), entity);
		if (found != ent_list.end())
		{
			const auto message = "The requested entityid is already attached to this system. EntityId: "
				+ to_string(entity) + ", System: " + "err" + ", at time: " +
				to_string(std::chrono::duration_cast<seconds_float>(t.time_since_epoch()).count()) + "s";
			//ent is already attached
			throw system_already_attached{message};
		}

		ent_list.emplace_back(entity);
		ents.insert(t, ent_list);
		system.attached_entities = ents;
	}

	void GameInterface::detachSystem(entity_id entity, unique_id sys, time_point t)
	{
		const auto lock = std::lock_guard{ _system_list_mut };

		auto &system = FindSystem(sys);
		auto ents = system.attached_entities.get();
		auto ent_list = ents.get(t);
		auto found = std::find(ent_list.begin(), ent_list.end(), entity);
		if (found == ent_list.end())
		{
			const auto message = "The requested entityid isn't attached to this system. EntityId: "
				+ to_string(entity) + ", System: " + "err" + ", at time: " +
				to_string(std::chrono::duration_cast<seconds_float>(t.time_since_epoch()).count()) + "s";
			throw system_already_attached{ message };
		}

		ent_list.erase(found);
		ents.insert(t, ent_list);
		system.attached_entities = ents;
	}

	GameSystem& GameInterface::install_system(unique_id sys)
	{
		//we MUST already be locked before we get here
		assert(!_system_list_mut.try_lock());

		//never install a system more than once.
		assert(std::none_of(std::begin(_systems), std::end(_systems), [sys](const auto &system) {
			return system.system->id == sys;
		}));

		const auto new_system = hades::data::get<resources::system>(sys);
		return _systems.emplace_back(new_system);
	}

	GameSystem& GameInterface::FindSystem(unique_id id)
	{
		//we MUST already be locked before we get here
		assert(!_system_list_mut.try_lock());

		for (auto &s : _systems)
		{
			if (s.system->id == id)
				return s;
		}

		return install_system(id);
	}
}