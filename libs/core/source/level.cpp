#include "hades/level.hpp"

#include "hades/core_curves.hpp"
#include "hades/curve_extra.hpp"
#include "hades/data.hpp"
#include "hades/parser.hpp"
#include "hades/writer.hpp"

namespace hades
{
	constexpr auto zero_time = time_point{};

	template<typename T>
	static void add_curve(curve_data::curve_map<T> &c, entity_id id, const resources::curve *curve, const resources::curve_default_value &value)
	{
		if (c.find({ id, curve->id }) != std::end(c))
			c.erase({ id, curve->id });

		hades::curve<T> curve_instance{ curve->c_type };

		assert(std::holds_alternative<T>(value));

		assert(resources::is_set(value));
		assert(resources::is_curve_valid(*curve, value));

		const auto &val = std::get<T>(value);
		curve_instance.insert(zero_time, val);

		c.emplace(curve_index_t{ id, curve->id }, std::move(curve_instance));
	}

	static void add_curve_from_object(curve_data &c, entity_id id, const resources::curve *curve, resources::curve_default_value value)
	{
		using namespace resources;

		if (std::holds_alternative<std::monostate>(value))
			value = reset_default_value(*curve);

		if (!resources::is_curve_valid(*curve, value))
			throw std::runtime_error("unexpected curve type");

		std::visit([&](auto &&v) {
			using T = std::decay_t<decltype(v)>;
			add_curve(get_curve_list<T>(c), id, curve, value);
		}, value);
	}

	static std::size_t get_system_index(level_save::system_list &system_list, level_save::system_attachment_list &attach_list, const resources::system *s)
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

	static void add_object_to_save(curve_data &c, entity_id id, const resources::object *o)
	{
		for (const auto b : o->base)
			add_object_to_save(c, id, b);

		for (const auto[curve, value] : o->curves)
			add_curve_from_object(c, id, curve, value);
	}

	static void add_object_to_systems(level_save::system_list &system_list, level_save::system_attachment_list &attach_list, entity_id id, const resources::object *o)
	{
		assert(system_list.size() == attach_list.size());

		//add systems from ancestors
		for (const auto b : o->base)
			add_object_to_systems(system_list, attach_list, id, b);

		for (const auto s : o->systems)
		{
			const auto sys_index = get_system_index(system_list, attach_list, s);

			auto &attch = attach_list[sys_index];

			auto ent_list = attch.empty() ? resources::curve_types::collection_object_ref{} : attch.get(zero_time);

			if (std::find(std::begin(ent_list), std::end(ent_list), id) == std::end(ent_list))
			{
				ent_list.push_back(id);
				attch.set(zero_time, ent_list);
			}
		}
	}

	using namespace std::string_view_literals;
	static constexpr auto level_width_str = "width"sv;
	static constexpr auto level_height_str = "height"sv;
	static constexpr auto level_name_str = "name"sv;
	static constexpr auto level_description_str = "desc"sv;

	static constexpr auto level_regions_str = "regions"sv;

	static constexpr auto level_scripts_input = "player_input"sv;

	static constexpr auto level_tiles_layer_str = "tile-layer"sv;
	static constexpr auto level_terrain_str = "terrain"sv;

	static void write_regions_from_level(const level &l, data::writer &w)
	{
		//regions:
		//    - [name, r, g, b, a, x, y, w, h]

		if (l.regions.empty())
			return;

		w.start_sequence(level_regions_str);

		for(const auto &r : l.regions)
		{
			w.start_sequence();
			w.write(r.name);

			w.write(r.colour.r);
			w.write(r.colour.g);
			w.write(r.colour.b);
			w.write(r.colour.a);
			
			w.write(r.bounds.x);
			w.write(r.bounds.y);
			w.write(r.bounds.width);
			w.write(r.bounds.height);
			w.end_sequence();
		}

		w.end_sequence();
	}

	static void read_regions_into_level(const data::parser_node &p, level &l)
	{
		//regions:
		//    - [name, r, g, b, a, x, y, w, h]

		assert(l.regions.empty());

		const auto region_node = p.get_child(level_regions_str);

		if (!region_node)
			return;

		const auto regions = region_node->get_children();

		for (const auto &r : regions)
		{
			assert(r);
			const auto v = r->get_children();
			assert(v.size() == 9);

			const auto name = v[0]->to_string();

			const auto r = v[1]->to_scalar<uint8>(),
				g = v[2]->to_scalar<uint8>(),
				b = v[3]->to_scalar<uint8>(),
				a = v[4]->to_scalar<uint8>();

			const auto c = colour{ r, g, b, a };

			const auto x = v[5]->to_scalar<float>(),
				y = v[6]->to_scalar<float>(),
				w = v[7]->to_scalar<float>(),
				h = v[8]->to_scalar<float>();

			const auto rect = rect_float{ x, y, w, h };

			l.regions.emplace_back(level::region{ c, rect, name });
		}
	}

	void serialise(const level& l, data::writer& w)
	{
		//w.start_map(level_str);
		w.write(level_width_str, l.map_x);
		w.write(level_height_str, l.map_y);

		if (!l.name.empty())
			w.write(level_name_str, l.name);
		if (!l.description.empty())
			w.write(level_description_str, l.description);

		write_objects_from_level(l, w);

		//level scripts
		if (l.player_input_script != unique_id::zero)
			w.write(level_scripts_input, l.player_input_script);

		//write terrain info
		w.start_map(level_tiles_layer_str);
		write_raw_map(l.tile_map_layer, w);
		w.end_map();

		w.start_map(level_terrain_str);
		const auto raw = raw_terrain_map{ l.terrainset,
			l.terrain_vertex, l.terrain_layers
		};

		write_raw_terrain_map(raw, w);
		w.end_map();
		//write trigger data
		write_regions_from_level(l, w);

		//w.end_map();
	}

	string serialise(const level &l)
	{
		auto w = data::make_writer();
		assert(w);
		serialise(l, *w);
		return w->get_string();
	}

	level deserialise_level(std::string_view s)
	{
		const auto parser = data::make_parser(s);
		return deserialise_level(*parser);
	}

	level deserialise_level(data::parser_node& level_node)
	{
		auto l = level{};

		//level_node:
		//	name:
		//	desc:
		//	size:

		l.name = data::parse_tools::get_scalar<string>(level_node, level_name_str, l.name);
		l.description = data::parse_tools::get_scalar<string>(level_node, level_description_str, l.description);
		l.map_x = data::parse_tools::get_scalar<level_size_t>(level_node, level_width_str, l.map_x);
		l.map_y = data::parse_tools::get_scalar<level_size_t>(level_node, level_height_str, l.map_y);

		read_objects_into_level(level_node, l);

		//read trigger data
		read_regions_into_level(level_node, l);

		//level scripts
		l.player_input_script = data::parse_tools::get_unique(level_node, level_scripts_input, l.player_input_script);

		const auto tiles = level_node.get_child(level_tiles_layer_str);
		if (tiles)
			l.tile_map_layer = read_raw_map(*tiles);

		const auto terrain = level_node.get_child(level_terrain_str);
		if (terrain)
			std::tie(l.terrainset, l.terrain_vertex, l.terrain_layers) = read_raw_terrain_map(*terrain);

		return l;
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
				auto names = sv.names.empty() ? std::map<string, entity_id>{} : sv.names.get(zero_time);
				names.emplace(o.name_id, o.id);
				sv.names.set(zero_time, names);
			}

			//add curves from parents
			if (o.obj_type)
			{
				add_object_to_save(sv.curves, o.id, o.obj_type);
				//add the object type as a curve
				add_curve_from_object(sv.curves, o.id, get_object_type_curve(), o.obj_type->id);
			}

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