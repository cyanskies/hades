#include "Hades/level.hpp"

#include "hades/curve.hpp"
#include "hades/data.hpp"

namespace hades
{
	level_save make_save_from_level(level l)
	{
		level_save sv;

		const auto object_type_id = hades::data::get_uid("object_type");
		const auto object_type_curve = hades::data::get<resources::curve>(object_type_id);

		for (const auto &o : l.objects)
		{
			assert(o.id != NO_ENTITY);

			if (!o.name.empty())
				sv.names.insert({ o.name, o.id });

			if (o.obj_type)
			{
				curve<sf::Time, unique_id> c(curve_type::const_c);
				c.insert(sf::Time(), o.obj_type->id);
				sv.curves.uniqueCurves.create({ o.id, object_type_id }, c);
			}
		}

		sv.next_id = l.next_id;
		sv.source = std::move(l);
		return sv;
	}
}