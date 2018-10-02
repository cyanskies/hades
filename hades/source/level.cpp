#include "Hades/level.hpp"

#include "SFML/System/Time.hpp"

#include "hades/curve.hpp"
#include "hades/data.hpp"

namespace hades
{
	const auto zero_time = sf::Time{};

	template<typename T>
	void add_curve(curve_data::CurveMap<T> &c, EntityId id, const resources::curve *curve, const resources::curve_default_value &value)
	{
		if (c.exists({ id, curve->id }))
			c.erase({ id, curve->id });

		hades::curve<sf::Time, T> curve_instance{ curve->curve_type };

		assert(std::holds_alternative<T>(value));

		if (!resources::is_curve_valid(*curve))
			throw curve_error("curve is missing a value");

		if (!std::holds_alternative<T>(value))
			throw curve_error("curve has wrong type");

		const auto &val = std::get<T>(value);
		curve_instance.insert(zero_time, val);

		c.create({ id, curve->id }, curve_instance);
	}

	void add_curve_from_object(curve_data &c, EntityId id, const resources::curve *curve, const resources::curve_default_value &value)
	{
		using namespace resources;

		switch (curve->data_type)
		{
		case curve_variable_type::OBJECT_REF:
			[[fallthrough]];
		case curve_variable_type::int_t:
			add_curve(c.intCurves, id, curve, value);
			break;
		case curve_variable_type::FLOAT:
			add_curve(c.floatCurves, id, curve, value);
			break;
		case curve_variable_type::BOOL:
			add_curve(c.boolCurves, id, curve, value);
			break;
		case curve_variable_type::STRING:
			add_curve(c.stringCurves, id, curve, value);
			break;
		case curve_variable_type::UNIQUE:
			add_curve(c.uniqueCurves, id, curve, value);
			break;
		case curve_variable_type::VECTOR_OBJECT_REF:
			[[fallthrough]];
		case curve_variable_type::VECTOR_INT:
			add_curve(c.intVectorCurves, id, curve, value);
			break;
		case curve_variable_type::VECTOR_FLOAT:
			add_curve(c.floatVectorCurves, id, curve, value);
			break;
		case curve_variable_type::VECTOR_UNIQUE:
			add_curve(c.uniqueVectorCurves, id, curve, value);
			break;
		default:
			throw std::runtime_error("unexpected curve type");
		}
	}

	std::size_t get_system_index(level_save::system_list &system_list, level_save::system_attachment_list &attach_list, const resources::system *s)
	{
		//search list
		const auto sys_iter = std::find(std::begin(system_list), std::end(system_list), s);
		auto pos = std::distance(std::begin(system_list), sys_iter);
		if (sys_iter == std::end(system_list))
		{
			pos = std::distance(std::begin(system_list), system_list.emplace(sys_iter, s));
			attach_list.emplace_back(curve_type::step);
		}

		return pos;
	}

	void add_object_to_save(curve_data &c, EntityId id, const objects::resources::object *o)
	{
		for (const auto b : o->base)
			add_object_to_save(c, id, b);

		for (const auto[curve, value] : o->curves)
			add_curve_from_object(c, id, curve, value);
	}

	void add_object_to_systems(level_save::system_list &system_list, level_save::system_attachment_list &attach_list, EntityId id, const objects::resources::object *o)
	{
		assert(system_list.size() == attach_list.size());

		//add systems from ancestors
		for (const auto b : o->base)
			add_object_to_systems(system_list, attach_list, id, b);

		for (const auto s : o->systems)
		{
			const auto sys_index = get_system_index(system_list, attach_list, s);

			auto &attch = attach_list[sys_index];

			auto ent_list = attch.empty() ? resources::curve_types::vector_int{} : attch.get(sf::Time{});

			if (std::find(std::begin(ent_list), std::end(ent_list), id) == std::end(ent_list))
			{
				ent_list.push_back(id);
				attch.set(sf::Time{}, ent_list);
			}
		}
	}

	level_save make_save_from_level(level l)
	{
		level_save sv;

		for (const auto &o : l.objects)
		{
			assert(o.id != NO_ENTITY);

			//record entity name if present
			if (!o.name.empty())
			{
				auto names = sv.names.get(sf::Time{});
				names.emplace(o.name, o.id);
				sv.names.set(sf::Time{}, names);
			}

			//add curves from parents
			if (o.obj_type)
				add_object_to_save(sv.curves, o.id, o.obj_type);

			//add curves from object info
			for (const auto[curve, value] : o.curves)
				add_curve_from_object(sv.curves, o.id, curve, value);

			//add object to systems
			add_object_to_systems(sv.systems, sv.systems_attached, o.id, o.obj_type);
		}

		sv.next_id = l.next_id;
		sv.source = std::move(l);
		return sv;
	}
}