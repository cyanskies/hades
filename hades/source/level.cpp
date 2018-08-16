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
		assert(value.set);
		assert(std::holds_alternative<T>(value.value));
		const auto &val = std::get<T>(value.value);
		curve_instance.insert(zero_time, val);

		c.create({ id, curve->id }, curve_instance);
	}

	void add_curve_from_object(curve_data &c, EntityId id, const resources::curve *curve, const resources::curve_default_value &value)
	{
		using namespace resources;

		switch (curve->data_type)
		{
		case OBJECT_REF:
			[[fallthrough]];
		case INT:
			add_curve(c.intCurves, id, curve, value);
			break;
		case FLOAT:
			add_curve(c.floatCurves, id, curve, value);
			break;
		case BOOL:
			add_curve(c.boolCurves, id, curve, value);
			break;
		case STRING:
			add_curve(c.stringCurves, id, curve, value);
			break;
		case UNIQUE:
			add_curve(c.uniqueCurves, id, curve, value);
			break;
		case VECTOR_OBJECT_REF:
			[[fallthrough]];
		case VECTOR_INT:
			add_curve(c.intVectorCurves, id, curve, value);
			break;
		case VECTOR_FLOAT:
			add_curve(c.floatVectorCurves, id, curve, value);
			break;
		case VECTOR_UNIQUE:
			add_curve(c.uniqueVectorCurves, id, curve, value);
			break;
		default:
			throw std::runtime_error("unexpected curve type");
		}
	}

	void add_object_to_save(curve_data &c, EntityId id, const objects::resources::object *o)
	{
		for (const auto b : o->base)
			add_object_to_save(c, id, b);

		for (const auto[curve, value] : o->curves)
			add_curve_from_object(c, id, curve, value);
	}

	level_save make_save_from_level(level l)
	{
		level_save sv;

		for (const auto &o : l.objects)
		{
			assert(o.id != NO_ENTITY);

			//record entity name if present
			if (!o.name.empty())
				sv.names.push_back({ o.name, o.id });

			//add curves from parents
			if (o.obj_type)
				add_object_to_save(sv.curves, o.id, o.obj_type);

			//add curves from object info
			for (const auto[curve, value] : o.curves)
				add_curve_from_object(sv.curves, o.id, curve, value);
		}

		sv.next_id = l.next_id;
		sv.source = std::move(l);
		return sv;
	}
}