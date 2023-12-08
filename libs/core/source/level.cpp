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

	using namespace std::string_view_literals;
	static constexpr auto level_width_str = "width"sv;
	static constexpr auto level_height_str = "height"sv;
	static constexpr auto level_name_str = "name"sv;
	static constexpr auto level_description_str = "desc"sv;

	static constexpr auto level_regions_str = "regions"sv;

	static constexpr auto level_load_str = "on-load-script"sv;
	static constexpr auto level_scripts_input = "player-input"sv;

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

            w.write(r.display_colour.r);
            w.write(r.display_colour.g);
            w.write(r.display_colour.b);
            w.write(r.display_colour.a);
			
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
		if (l.on_load != unique_zero)
			w.write(level_load_str, l.on_load);
		if (l.player_input_script != unique_id::zero)
			w.write(level_scripts_input, l.player_input_script);

		//write terrain info
		w.start_map(level_tiles_layer_str);
		write_raw_map(l.tile_map_layer, w);
		w.end_map();

		w.start_map(level_terrain_str);
		// TODO: the tile layer isn't being written
		const auto raw = raw_terrain_map{ l.terrainset,
            l.terrain_vertex, l.height_vertex, l.terrain_layers,
            {}
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
		l.on_load = data::parse_tools::get_unique(level_node, level_load_str, l.on_load);
		l.player_input_script = data::parse_tools::get_unique(level_node, level_scripts_input, l.player_input_script);

		const auto tile_size = resources::get_tile_size();
		const auto layer_size = (l.map_x / tile_size) * (l.map_y / tile_size);

		const auto tiles = level_node.get_child(level_tiles_layer_str);
		if (tiles)
			l.tile_map_layer = read_raw_map(*tiles, layer_size);

		const auto terrain = level_node.get_child(level_terrain_str);
		if (terrain)
			std::tie(l.terrainset, l.terrain_vertex, l.height_vertex, l.terrain_layers) = 
			read_raw_terrain_map(*terrain, layer_size,
				(static_cast<size_t>(l.map_x / tile_size) + 1) * (static_cast<std::size_t>(l.map_y / tile_size) + 1));

		return l;
	}

	level_save make_save_from_level(level l)
	{
		level_save sv;

		//convert objects to save format
		sv.objects.next_id = l.objects.next_id;
		sv.objects.objects.reserve(std::size(l.objects.objects));
		for (auto& obj : l.objects.objects)
			sv.objects.objects.push_back(make_save_instance(std::move(obj)));

		sv.source = std::move(l);
		return sv;
	}
}
