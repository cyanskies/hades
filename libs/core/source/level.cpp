#include "hades/level.hpp"

#include "hades/curve_extra.hpp"
#include "hades/data.hpp"
#include "hades/writer.hpp"

namespace hades
{
	const auto zero_time = time_point{};

	template<typename T>
	void add_curve(curve_data::curve_map<T> &c, entity_id id, const resources::curve *curve, const resources::curve_default_value &value)
	{
		if (c.exists({ id, curve->id }))
			c.erase({ id, curve->id });

		hades::curve<T> curve_instance{ curve->curve_type };

		assert(std::holds_alternative<T>(value));

		if (!resources::is_set(value))
			throw curve_error("curve is missing a value");

		if (!resources::is_curve_valid(*curve, value))
			throw curve_error("curve has wrong type");

		const auto &val = std::get<T>(value);
		curve_instance.insert(zero_time, val);

		c.create({ id, curve->id }, curve_instance);
	}

	void add_curve_from_object(curve_data &c, entity_id id, const resources::curve *curve, const resources::curve_default_value &value)
	{
		using namespace resources;

		if (!resources::is_curve_valid(*curve, value))
			throw std::runtime_error("unexpected curve type");

		std::visit([&](auto &&v) {
			using T = std::decay_t<decltype(v)>;
			add_curve(get_curve_list<T>(c), id, curve, value);
		}, value);
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

	void add_object_to_save(curve_data &c, entity_id id, const resources::object *o)
	{
		for (const auto b : o->base)
			add_object_to_save(c, id, b);

		for (const auto[curve, value] : o->curves)
			add_curve_from_object(c, id, curve, value);
	}

	void add_object_to_systems(level_save::system_list &system_list, level_save::system_attachment_list &attach_list, entity_id id, const resources::object *o)
	{
		assert(system_list.size() == attach_list.size());

		//add systems from ancestors
		for (const auto b : o->base)
			add_object_to_systems(system_list, attach_list, id, b);

		for (const auto s : o->systems)
		{
			const auto sys_index = get_system_index(system_list, attach_list, s);

			auto &attch = attach_list[sys_index];

			auto ent_list = attch.empty() ? resources::curve_types::vector_object_ref{} : attch.get(zero_time);

			if (std::find(std::begin(ent_list), std::end(ent_list), id) == std::end(ent_list))
			{
				ent_list.push_back(id);
				attch.set(zero_time, ent_list);
			}
		}
	}

	string serialise(const level &l)
	{
		using namespace std::string_view_literals;
		auto w = data::make_writer();
		assert(w);

		w->start_map("level"sv);
		w->write("width"sv, l.map_x);
		w->write("height"sv, l.map_y);

		if(!l.name.empty())
			w->write("name", l.name);
		if(!l.description.empty())
			w->write("description", l.description);

		write_objects_from_level(l, *w);
		//write terrain info

		w->end_map();
		return w->get_string();
	}

	level deserialise(const string &s)
	{
		//TODO: make_default_parser?
		return level();
	}

	level_save make_save_from_level(level l)
	{
		level_save sv;

		for (const auto &o : l.objects)
		{
			assert(o.id != bad_entity);

			//record entity name if present
			if (!o.name_id.empty())
			{
				auto names = sv.names.get(zero_time);
				names.emplace(o.name_id, o.id);
				sv.names.set(zero_time, names);
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