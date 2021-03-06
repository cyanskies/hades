#include "hades/level.hpp"

#include "hades/core_curves.hpp"
#include "hades/curve_extra.hpp"
#include "hades/data.hpp"
#include "hades/game_state.hpp"
#include "hades/parser.hpp"
#include "hades/writer.hpp"

namespace hades
{
	constexpr auto zero_time = time_point{};

	namespace
	{
		struct add_curve_visitor
		{
			game_state::state_data_type& c;
			entity_id id;
			const resources::curve* curve;

			template<typename Ty>
			void operator()(Ty& v)
			{
				using T = std::decay_t<decltype(v)>;
				using ColonyType = game_state::data_colony<T>;
				auto& colony = std::get<ColonyType>(c);
				colony.insert(state_field<T>{ id, curve->id, std::move(v) });
				return;
			}

			template<> // calling this should be impossible
			void operator()<std::monostate> (std::monostate&) { return; }
		};
	}

	static void add_curve_from_object(game_state::state_data_type &c, entity_id id, const resources::curve *curve, resources::curve_default_value value)
	{
		using namespace resources;

		if (std::holds_alternative<std::monostate>(value))
			value = reset_default_value(*curve);

		// TODO: FIXME:
		//	replace with better exception type
		if (!resources::is_curve_valid(*curve, value))
			throw std::runtime_error("unexpected curve type");

		std::visit(add_curve_visitor{ c, id, curve }, value);
	}

	//static std::size_t get_system_index(level_save::system_list &system_list, level_save::system_attachment_list &attach_list, const resources::system *s)
	//{
	//	//search list
	//	const auto sys_iter = std::find(std::begin(system_list), std::end(system_list), s);
	//	auto pos = std::distance(std::begin(system_list), sys_iter);
	//	if (sys_iter == std::end(system_list))
	//	{
	//		pos = std::distance(std::begin(system_list), system_list.emplace(sys_iter, s));
	//		attach_list.emplace_back();
	//	}

	//	return pos;
	//}

	static bool empty_obj(const resources::object* o)
	{
		return empty(o->base) &&
			empty(o->curves) &&
			empty(o->editor_anims) &&
			(o->editor_icon == nullptr) &&
			empty(o->render_systems) &&
			empty(o->systems);
	}

	static void add_object_to_save(game_state::state_data_type &c, entity_id id, const resources::object *o)
	{
		assert(o);
		if (empty_obj(o))
			LOGWARNING("object type: " + to_string(o->id) + ", has no systems, curves, or base objects.");

		for (const auto b : o->base)
			add_object_to_save(c, id, b);

		for (const auto[curve, value] : o->curves)
			add_curve_from_object(c, id, curve, value);
	}

	//static void add_object_to_systems(level_save::system_list &system_list, level_save::system_attachment_list &attach_list, entity_id id, const resources::object *o)
	//{
	//	assert(system_list.size() == attach_list.size());

	//	//add systems from ancestors
	//	for (const auto b : o->base)
	//		add_object_to_systems(system_list, attach_list, id, b);

	//	for (const auto s : o->systems)
	//	{
	//		const auto sys_index = get_system_index(system_list, attach_list, s);
	//		auto& ent_list = attach_list[sys_index];

	//		if (std::find(std::begin(ent_list), std::end(ent_list), id) == std::end(ent_list))
	//			ent_list.push_back(id);
	//	}
	//}

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

		for (const auto &region : regions)
		{
			assert(region);
			const auto v = region->get_children();
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

		serialise(l.objects, w);

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

		l.objects = deserialise_object_data(level_node);

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
		sv.objects = l.objects; // copy
		sv.source = std::move(l);
		return sv;
	}
}