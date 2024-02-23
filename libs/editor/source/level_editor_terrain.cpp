#include "hades/level_editor_terrain.hpp"

#include "hades/camera.hpp"
#include "hades/gui.hpp"
#include "hades/level_editor.hpp"
#include "hades/properties.hpp"
#include "hades/terrain.hpp"

#include "hades/random.hpp" // temp; for generating heightmaps

namespace hades
{
	using namespace std::string_literals;
	void register_level_editor_terrain_resources(data::data_manager &d)
	{
		register_terrain_map_resources(d);
	}

	void create_level_editor_terrain_variables()
	{
		console::create_property(cvars::editor_default_terrainset, cvars::default_value::editor_default_terrainset);
		return;
	}

	level_editor_terrain::level_editor_terrain() :
		_settings{ resources::get_terrain_settings() },
		_view_height{ console::get_int(cvars::editor_camera_height_px, cvars::default_value::editor_camera_height_px) }
	{
		assert(!empty(_settings->empty_tileset.get()->tiles));
		_empty_tile = _settings->empty_tileset.get()->tiles[0];
		_empty_terrain = _settings->empty_terrain.get();
		_resize.terrain = _empty_terrain;
		_empty_terrainset = _settings->empty_terrainset.get();
		_tile_size = _settings->tile_size;

		// load all terrain sets
		std::ranges::for_each(_settings->terrainsets, [](auto id) {
			data::get<resources::terrainset>(id);
			}, &resources::terrainset::id);

		//default terrain set
		const auto terrainset_name = console::get_string(cvars::editor_default_terrainset, cvars::default_value::editor_default_terrainset);
		if (!empty(terrainset_name->load()))
		{
			const auto id = data::get_uid(terrainset_name->load());
			try
			{
				_new_options.terrain_set = data::get<resources::terrainset>(id);
			}
			catch (const data::resource_error& e)
			{
				LOG("cannot load editor default terrainset: "s + to_string(e.what()));
			}
		}
		
		if (_new_options.terrain_set == nullptr)
		{
			if (!empty(_settings->terrainsets))
				_new_options.terrain_set = _settings->terrainsets.front().get();
		}

		if (_new_options.terrain_set &&
			_new_options.terrain == nullptr &&
			!empty(_new_options.terrain_set->terrains))
		{
			_new_options.terrain = _new_options.terrain_set->terrains.front().get();
		}
	}

	level level_editor_terrain::level_new(level current) const
	{
		if (!_new_options.terrain_set ||
			!_new_options.terrain)
		{
			const auto msg = "must select a terrain set and a starting terrain"s;
			LOGERROR(msg);
			throw new_level_editor_error{ msg };
		}

		if (current.map_x % _settings->tile_size != 0 ||
			current.map_y % _settings->tile_size != 0)
		{
			const auto msg = "level size must be a multiple of tile size("s
				+ to_string(_settings->tile_size) +")"s;

			LOGERROR(msg);
			throw new_level_editor_error{ msg };
		}
				
		const auto size = tile_position{
			signed_cast(current.map_x / _settings->tile_size),
			signed_cast(current.map_y / _settings->tile_size)
		};

		auto map = make_map(size, _new_options.terrain_set, _new_options.terrain, *_settings);
		
		constexpr auto variance = 20;
		const auto height_low = _settings->height_default - variance;
		const auto height_high = height_low + variance * 2;

		const auto vert_size = get_vertex_size(map);
		for_each_position_rect({}, vert_size, vert_size, [&](const tile_position p) {
			set_height_at(map, p, integer_clamp_cast<std::uint8_t>(random(height_low, height_high)), _settings);
			return;
			});

		auto l = level{ std::move(current) };
		l.terrain = to_raw_terrain_map(std::move(map));

		return l;
	}

	void level_editor_terrain::level_load(const level &l)
	{
		auto map_raw = l.terrain;

		//check the scale and size of the map
		assert(_settings);
		const auto tile_size = _settings->tile_size;

		const auto size = tile_position{
			signed_cast(l.map_x / tile_size),
			signed_cast(l.map_y / tile_size)
		};

		assert(size.x >= 0 && size.y >= 0);

		//change the raw map so it can be validly converted into a terrain_map
		//generate a empty tile layer of the correct size
		if (std::empty(map_raw.tile_layer.tiles) ||
			map_raw.tile_layer.width == tile_index_t{})
		{
			map_raw.tile_layer.width = integer_cast<tile_index_t>(size.x);
			map_raw.tile_layer.tilesets.clear();
			map_raw.tile_layer.tilesets.emplace_back(resources::get_empty_tileset_id(), 0u);

 			map_raw.tile_layer.tiles = std::vector<tile_id_t>(
				static_cast<std::size_t>(size.x) * static_cast<std::size_t>(size.y),
				tile_id_t{}
			);
		}

		//if terrainset is empty then assign one
		if (map_raw.terrainset == unique_id::zero)
		{
			if (std::empty(_settings->terrainsets))
				map_raw.terrainset = _settings->empty_terrainset->id;
			else
				map_raw.terrainset = _settings->terrainsets.front()->id;
		}

		auto map = to_terrain_map(std::move(map_raw));
		//TODO: if size != to level xy, then resize the map
		auto empty_map = make_map(size, _settings->editor_terrain ? _settings->editor_terrainset.get() : map.terrainset,
			_empty_terrain, *_settings);
		copy_heightmap(empty_map, map);

		_current.terrain_set = map.terrainset;
		_map.reset(std::move(map));
		_map.set_sun_angle(_sun_angle); // TODO: temp
		// _sun_angle = get_sun_angle(map);
		_clear_preview.reset(std::move(empty_map));
		_clear_preview.show_shadows(false);
		return;
	}

	level level_editor_terrain::level_save(level l) const
	{
		l.terrain = to_raw_terrain_map(_map.get_map());
		return l;
	}

	void level_editor_terrain::level_resize(vector2_int s, vector2_int o)
	{
		terrain_map map = _map.get_map(); // make copy of map
		const auto new_size_tiles = to_tiles(s, _tile_size);
		const auto offset_tiles = to_tiles(o, _tile_size);

		// TODO: ui to select new height
		resize_map(map, new_size_tiles, offset_tiles, _resize.terrain, _settings->height_default, *_settings);
		auto empty_map = make_map(new_size_tiles + vector2_int{ 1, 1 }, map.terrainset, _empty_terrain, *_settings);
		_clear_preview.reset(std::move(empty_map));
		_map.reset(std::move(map));
		_resize.terrain = _empty_terrain;
	}

	void level_editor_terrain::gui_update(gui &g, editor_windows &w)
	{
		using namespace std::string_view_literals;

		if (g.main_toolbar_begin())
		{			
			if (g.toolbar_button("terrain eraser"sv))
			{
				activate_brush();
				_brush = brush_type::erase;
			}

			if (g.toolbar_button("terrain grid"sv))
			{
				_map.show_grid(!_map.show_grid());
			}

			if (g.toolbar_button("terrain depth"sv))
			{
				_map.draw_depth_buffer(!_map.draw_depth_buffer());
			}
		}

		g.main_toolbar_end();

		constexpr auto button_size = gui::vector2{
			25.f,
			25.f
		};

		const auto tile_size_f = float_cast(_tile_size);

		const auto make_terrain_button_wrapped = [this, &g, tile_size_f](auto&& terrain, auto&& func) {
			if (empty(terrain->tiles))
				return;

			const auto& t = terrain->tiles.front();

			const auto x = static_cast<float>(t.left),
				y = static_cast<float>(t.top);

			const auto tex_coords = rect_float{
				x,
				y,
				tile_size_f,
				tile_size_f
			};

			//need to push a prefix to avoid the id clashing from the same texture
			g.push_id(terrain.get());
			const auto size = g.calculate_button_size({}, button_size);
			g.same_line_wrapping(size.x);
			if (g.image_button("###terrain_button"sv, *t.tex, tex_coords, button_size))
				std::invoke(func, terrain.get());
			g.tooltip(data::get_as_string(terrain->id));
			g.pop_id();
			return;
		};

		if (g.window_begin(editor::gui_names::toolbox))
		{
			if (g.collapsing_header("sun settings"sv))
			{
				if (g.slider_scalar("Sun angle"sv, _sun_angle, 180.f, 0.f))
					_map.set_sun_angle(_sun_angle);
			}

			if (g.collapsing_header("map drawing settings"sv))
			{
				constexpr auto draw_shapes = std::array{
					"vertex"sv,
					"edge"sv,
					"square"sv,
					"circle"sv
				};

				if (g.combo_begin("drawing shape"sv, draw_shapes[static_cast<std::size_t>(_shape)]))
				{
					for (auto i = std::size_t{}; i < size(draw_shapes); ++i)
					{
						const auto shape = draw_shape{ integer_cast<std::underlying_type_t<draw_shape>>(i) };
						if (g.selectable(draw_shapes[i], _shape == shape))
							_shape = shape;
					}

					g.combo_end();
				}

				constexpr auto size_min = int{ 1 };
				constexpr auto size_max = int{ 6 };

				g.input_scalar("drawing size"sv, _size);
				_size = std::clamp(_size, size_min, size_max);
			}

			if (g.collapsing_header("height"sv))
			{
				g.slider_scalar("Amount"sv, _height_strength, std::uint8_t{ 1 }, std::uint8_t{ 10 });

				if (g.button("Raise"sv))
				{
					activate_brush();
					_brush = brush_type::raise_terrain;
				}

				if (g.button("Lower"sv))
				{
					activate_brush();
					_brush = brush_type::lower_terrain;
				}
			}

			if (g.collapsing_header("tiles"sv))
			{
				//each tileset
				//the tiles from a terrain wont appear here unless
				//they are listed under the tiles: tag in the terrain
				for (const auto& tileset : _settings->tilesets)
				{
					const auto& name = data::get_as_string(tileset->id);

					g.indent();
					if (g.collapsing_header(name))
					{
						for (const auto &t : tileset->tiles)
						{
							const auto x = static_cast<float>(t.left),
								y = static_cast<float>(t.top);

							const auto tex_coords = rect_float{
								x,
								y,
								tile_size_f,
								tile_size_f
							};

							//need to push a prefix to avoid the id clashing from the same texture
							g.push_id(&t);
							const auto siz = g.calculate_button_size({}, button_size);
							g.same_line_wrapping(siz.x);
							if (g.image_button("###tile_button"sv, *t.tex, tex_coords, button_size))
							{
								activate_brush();
								_brush = brush_type::draw_tile;
								_tile = t;
							}
							g.pop_id();
						}
					}
				}
			}

			if (g.collapsing_header("terrain"sv))
			{
				assert(_current.terrain_set);

				if (g.button("empty"sv))
				{
					activate_brush();
					_brush = brush_type::draw_terrain;
					_current.terrain = _empty_terrain;
				}

				//if we have the empty terrainset then skip this,
				// the menu would be useless anyway
				if (_current.terrain_set != _empty_terrainset)
				{
					for (const auto& ter : _current.terrain_set->terrains)
					{
						make_terrain_button_wrapped(ter, [&](auto&& t) {
							activate_brush();
							_brush = brush_type::draw_terrain;
							_current.terrain = t;
						});
					}
				}
			}
		}
		g.window_end();
		
		if (w.new_level)
		{
			if (g.window_begin(editor::gui_names::new_level))
			{
				using namespace std::string_literals;
				auto string = "none"s;
				if (_new_options.terrain_set != nullptr)
					string = data::get_as_string(_new_options.terrain_set->id);

				if (g.combo_begin("terrain set"sv, string))
				{
					for (const auto& tset : _settings->terrainsets)
					{
						assert(tset);
						if (g.selectable(data::get_as_string(tset->id), tset.get() == _new_options.terrain_set))
							_new_options.terrain_set = tset.get();
					}

					g.combo_end();
				}

				g.text("Fill new level with: "sv);
				g.same_line();

				if (_new_options.terrain_set != nullptr)
				{
					g.text(data::get_as_string(_new_options.terrain->id));

					if (g.button("empty"sv))
						_new_options.terrain = _empty_terrain;

					for (const auto& ter : _new_options.terrain_set->terrains)
					{
						make_terrain_button_wrapped(ter, [&](auto&& t) {
							_new_options.terrain = t;
							});
					}
				}
				else
					g.text("empty"sv);

			}
			g.window_end();
		}

		if (w.resize_level)
		{
			if (g.window_begin(editor::gui_names::resize_level))
			{
				g.text("Fill new areas with: "sv);
				g.same_line();
				g.text(data::get_as_string(_resize.terrain->id));

				if (g.button("empty"sv))
					_resize.terrain = _empty_terrain;
			
				for (const auto& ter : _current.terrain_set->terrains)
				{
					make_terrain_button_wrapped(ter, [&](auto&& t) {
						_resize.terrain = t;
						});
				}
			}

			g.window_end();
		}
	}

	template<typename Func>
	static void for_each_position(const level_editor_terrain::mouse_pos p,
		const resources::tile_size_t tile_size, const level_editor_terrain::draw_shape shape,
		int size, const tile_position world_size, Func &&f)
	{
		const auto draw_pos = tile_position{
			static_cast<tile_position::value_type>(p.x / tile_size),
			static_cast<tile_position::value_type>(p.y / tile_size)
		};

		// TODO: within_map
		if (!within_world(draw_pos, world_size))
			return;

		using draw_shape = level_editor_terrain::draw_shape;

		switch (shape)
		{
		case draw_shape::vertex:
			// round to nearest vertex(in map)
			return for_each_safe_position_rect(draw_pos, tile_position{ 1, 1 }, world_size, f);
		case draw_shape::edge:
		{

			return;
		}
		case draw_shape::rect:
			++size;
			return for_each_safe_position_rect(draw_pos, tile_position{ size, size }, world_size, f);
		case draw_shape::circle:
			return for_each_safe_position_circle(draw_pos, size, world_size, f);
		}

		return;
	}

	void level_editor_terrain::make_brush_preview(time_duration, mouse_pos p)
	{
		_preview = _clear_preview;

		const auto func = [&](const tile_position pos) {
			switch (_brush)
			{
			case brush_type::erase:
				[[fallthrough]];
			case brush_type::raise_terrain:
				[[fallthrough]];
			case brush_type::lower_terrain:
				[[fallthrough]];
			case brush_type::draw_terrain:
				_preview.place_terrain(pos, _settings->editor_terrain ? _settings->editor_terrain.get() : _current.terrain);
				break;
			case brush_type::draw_tile:
				_preview.place_tile(pos, _tile);
				break;
			}	
		};

		for_each_position(p, _settings->tile_size, _shape, _size, get_size(_map.get_map()) + tile_position{ 1, 1 }, func);
		return;
	}

	tag_list level_editor_terrain::get_terrain_tags_at_location(rect_float location) const
	{
		//TODO: maybe move this to utility?
		constexpr auto snap_to_floor = [](float val, int mul)->float {
			const auto diff = std::remainder(val, static_cast<float>(mul));
			return std::trunc(val - diff);
		};

		const auto tile_size = signed_cast(_settings->tile_size);
		const auto loc = rect_int{
			static_cast<int>(snap_to_floor(location.x, tile_size)) / tile_size,
			static_cast<int>(snap_to_floor(location.y, tile_size)) / tile_size,
			static_cast<int>(snap_to_floor(location.width + tile_size, tile_size)) / tile_size,
			static_cast<int>(snap_to_floor(location.height + tile_size, tile_size)) / tile_size,
		};

		const auto positions = make_position_rect({ loc.x, loc.y }, { loc.width, loc.height });

		auto out = tag_list{};
		for (const tile_position p : positions)
		{
			const auto tags = hades::get_tags_at(_map.get_map(), p);
			out.insert(std::end(out), std::begin(tags), std::end(tags));
		}

		return out;
	}

	void level_editor_terrain::on_reinit(vector2_float window_size, vector2_float view_size)
	{
		auto view_height = static_cast<float>(*_view_height) * editor::zoom_max;
		auto default_w = camera::calculate_width(static_cast<float>(*_view_height), window_size.x, window_size.y);
		const auto max_zoom = 3.f;
		const auto chunk_count = 4;
		auto chunk_size = std::max(view_height * max_zoom, default_w * max_zoom) / 2.f; //view is observed from centre
		chunk_size += 10.f;
		//_map.set_chunk_size(integral_cast<std::size_t>(chunk_size, round_up_tag));

		//also do screen move
	}

	void level_editor_terrain::on_click(mouse_pos p)
	{
		auto func = [&](const tile_position pos) {
			switch (_brush)
			{
			case brush_type::draw_terrain:
				_map.place_terrain(pos, _current.terrain);
				break;
			case brush_type::draw_tile:
				_map.place_tile(pos, _tile);;
				break;
			case brush_type::erase:
				_map.place_terrain(pos, _empty_terrain);
				break;
			case brush_type::raise_terrain:
				_map.raise_terrain(pos, _height_strength);
				_clear_preview.raise_terrain(pos, _height_strength);
				break;
			case brush_type::lower_terrain:
				_map.lower_terrain(pos, _height_strength);
				_clear_preview.lower_terrain(pos, _height_strength);
				break;
			}	
		};

		// NOTE: expand map size by one again, since the tile based for_each_safe_pos is written around tiles not vertex
		//		(it uses < operators for the right and bottom limits, but we would need it to behave like <=
		for_each_position(p, _settings->tile_size, _shape, _size, get_size(_map.get_map()) + tile_position{ 1, 1 }, func);
		return;
	}

	void level_editor_terrain::on_drag(mouse_pos p)
	{
		on_click(p);
	}


	void level_editor_terrain::on_screen_move(rect_float r)
	{
		_map.set_world_region(r);
		_clear_preview.set_world_region(r);
		_preview.set_world_region(r);
		return;
	}

	void level_editor_terrain::on_height_toggle(bool b) noexcept
	{
		_map.set_height_enabled(b);
		_preview.set_height_enabled(b);
		_clear_preview.set_height_enabled(b);
		return;
	}

	void level_editor_terrain::draw(sf::RenderTarget &r, time_duration, sf::RenderStates s)
	{
		_map.apply();
		_map.set_world_rotation(get_world_rotation());
		r.draw(_map, s);
	}

	void level_editor_terrain::draw_brush_preview(sf::RenderTarget &r, time_duration, sf::RenderStates s)
	{
		_preview.apply();
		_preview.set_world_rotation(get_world_rotation());
		r.draw(_preview, s);
	}
}
