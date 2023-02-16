#include "hades/level_editor_terrain.hpp"

#include "hades/gui.hpp"
#include "hades/properties.hpp"
#include "hades/terrain.hpp"

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
		_settings{resources::get_terrain_settings()}
	{
		assert(!empty(_settings->empty_tileset.get()->tiles));
		_empty_tile = _settings->empty_tileset.get()->tiles[0];
		_empty_terrain = _settings->empty_terrain.get();
		_empty_terrainset = _settings->empty_terrainset.get();
		_tile_size = _settings->tile_size;

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
			_new_options.terrain = _new_options.terrain_set->terrains.back().get();
		}
	}

	level level_editor_terrain::level_new(level l) const
	{
		if (!_new_options.terrain_set ||
			!_new_options.terrain)
		{
			const auto msg = "must select a terrain set and a starting terrain"s;
			LOGERROR(msg);
			throw new_level_editor_error{ msg };
		}

		if (l.map_x % _settings->tile_size != 0 ||
			l.map_y % _settings->tile_size != 0)
		{
			const auto msg = "level size must be a multiple of tile size("s
				+ to_string(_settings->tile_size) +")"s;

			LOGERROR(msg);
			throw new_level_editor_error{ msg };
		}
				
		const auto size = tile_position{
			signed_cast(l.map_x / _settings->tile_size),
			signed_cast(l.map_y / _settings->tile_size)
		};

		const auto map = make_map(size, _new_options.terrain_set, _new_options.terrain);
		const auto raw = to_raw_terrain_map(map);
		l.terrainset = raw.terrainset;
		l.terrain_layers = raw.terrain_layers;
		l.tile_map_layer = raw.tile_layer;
		l.terrain_vertex = raw.terrain_vertex;

		return l;
	}

	void level_editor_terrain::level_load(const level &l)
	{
		auto map_raw = raw_terrain_map{ l.terrainset,
			l.terrain_vertex,
			l.terrain_layers,
			l.tile_map_layer
		};

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

		auto map = to_terrain_map(map_raw);
		//TODO: if size != to level xy, then resize the map
		auto empty_map = make_map(size, map.terrainset, _empty_terrain);

		_current.terrain_set = map.terrainset;
		_map.create(std::move(map));
		_clear_preview.create(std::move(empty_map));
		return;
	}

	level level_editor_terrain::level_save(level l) const
	{
		auto raw = to_raw_terrain_map(_map.get_map());

		l.tile_map_layer = std::move(raw.tile_layer);
		l.terrainset = raw.terrainset;
		l.terrain_vertex = std::move(raw.terrain_vertex);
		l.terrain_layers = std::move(raw.terrain_layers);

		return l;
	}

	void level_editor_terrain::level_resize(vector_int s, vector_int o)
	{
		terrain_map map = _map.get_map(); // make copy of map
		const auto new_size_tiles = to_tiles(s, _tile_size);
		const auto offset_tiles = to_tiles(o, _tile_size);

		resize_map(map, new_size_tiles, offset_tiles);
		auto empty_map = make_map(new_size_tiles, map.terrainset, _empty_terrain);
		_clear_preview.create(std::move(empty_map));
		_map.create(std::move(map));
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
		}

		g.main_toolbar_end();

		if (g.window_begin(editor::gui_names::toolbox))
		{
			const auto toolbox_width = g.get_item_rect_max().x;

			if (g.collapsing_header("map drawing settings"sv))
			{
				constexpr auto draw_shapes = std::array{
					"square"sv,
					"circle"sv
				};

				if (g.combo_begin("drawing shape"sv, draw_shapes[static_cast<std::size_t>(_shape)]))
				{
					if (g.selectable(draw_shapes[0], _shape == draw_shape::rect))
						_shape = draw_shape::rect;

					if (g.selectable(draw_shapes[1], _shape == draw_shape::circle))
						_shape = draw_shape::circle;

					g.combo_end();
				}

				constexpr auto size_min = int{ 0 };
				constexpr auto size_max = int{ 6 };

				g.input_scalar("drawing size"sv, _size);
				_size = std::clamp(_size, size_min, size_max);
			}

			if (g.collapsing_header("tiles"sv))
			{
				auto on_click = [this](resources::tile t) {
					activate_brush();
					_brush = brush_type::draw_tile;
					_tile = t;
				};

				auto make_button = [this, on_click](gui &g, const resources::tile &t) {
					constexpr auto button_size = gui::vector2{
						25.f,
						25.f
					};

					const auto x = static_cast<float>(t.left),
						y = static_cast<float>(t.top);

					const auto tile_size = static_cast<float>(_tile_size);

					const auto tex_coords = rect_float{
						x,
						y,
						tile_size,
						tile_size
					};

					//need to push a prefix to avoid the id clashing from the same texture
					g.push_id(&t);
					if (g.image_button(*t.tex, tex_coords, button_size))
						std::invoke(on_click, t);
					g.pop_id();
				};

				//each tileset
				//the tiles from a terrain wont appear here unless
				//they are listed under the tiles: tag in the terrain
				for (const auto tileset : _settings->tilesets)
				{
					const auto name = data::get_as_string(tileset->id);

					g.indent();
					if (g.collapsing_header(name))
						gui_make_horizontal_wrap_buttons(g, toolbox_width, std::begin(tileset->tiles), std::end(tileset->tiles), make_button);
				}
			}

			if (g.collapsing_header("terrain"sv))
			{
				auto on_click = [this](const resources::terrain *t) {
					activate_brush();
					_brush = brush_type::draw_terrain;
					_current.terrain = t;
				};

				auto make_button = [this, on_click](gui &g, resources::resource_link<resources::terrain> terrain) {
					constexpr auto button_size = gui::vector2{
						25.f,
						25.f
					};

					assert(!std::empty(terrain->tiles));
					const auto& t = terrain->tiles.front();

					const auto x = static_cast<float>(t.left),
						y = static_cast<float>(t.top);
					
					const auto tile_size = static_cast<float>(_tile_size);

					const auto tex_coords = rect_float{
						x,
						y,
						tile_size,
						tile_size
					};

					//need to push a prefix to avoid the id clashing from the same texture
					g.push_id(terrain);
					if (g.image_button(*t.texture, tex_coords, button_size))
						std::invoke(on_click, terrain.get());
					g.pop_id();
				};

				assert(_current.terrain_set);

				//if we have the empty terrainset then skip this,
				// the menu would be useless anyway
				if (_current.terrain_set != _empty_terrainset)
				{
					g.indent();
					gui_make_horizontal_wrap_buttons(g, toolbox_width,
						std::begin(_current.terrain_set->terrains),
						std::end(_current.terrain_set->terrains), make_button);
				}
			}
		}
		g.window_end();
		
		if (w.new_level)
		{
			if (g.window_begin(editor::gui_names::new_level))
			{
				auto dialog_pos = g.window_position().x;
				auto dialog_size = g.get_item_rect_max().x;
				
				using namespace std::string_literals;
				auto string = "none"s;
				if (_new_options.terrain_set != nullptr)
					string = data::get_as_string(_new_options.terrain_set->id);

				if (g.combo_begin("terrain set"sv, string))
				{
					for (const auto tset : _settings->terrainsets)
					{
						assert(tset);
						if (g.selectable(data::get_as_string(tset->id), tset.get() == _new_options.terrain_set))
							_new_options.terrain_set = tset.get();
					}

					g.combo_end();
				}

				if (_new_options.terrain_set != nullptr)
				{
					auto on_click = [this](const resources::terrain *t) {
						_new_options.terrain = t;
					};

					auto make_button = [this, on_click](gui &g, resources::resource_link<resources::terrain> terrain) {
						constexpr auto button_size = gui::vector2{
							25.f,
							25.f
						};

						//TODO: FIXME: hide empty tilesets from combobox rather than crash here
						assert(!std::empty(terrain->tiles));
						const auto& t = terrain->tiles.front();

						const auto x = static_cast<float>(t.left),
							y = static_cast<float>(t.top);

						const auto tile_size = static_cast<float>(_tile_size);

						const auto tex_coords = rect_float{
							x,
							y,
							tile_size,
							tile_size
						};

						//need to push a prefix to avoid the id clashing from the same texture
						g.push_id(terrain);
						if (g.image_button(*t.texture, tex_coords, button_size))
							std::invoke(on_click, terrain.get());
						g.pop_id();
					};

					if (g.button("empty"sv))
						std::invoke(on_click, _empty_terrain);

					gui_make_horizontal_wrap_buttons(g, dialog_pos + dialog_size,
						std::begin(_new_options.terrain_set->terrains),
						std::end(_new_options.terrain_set->terrains), make_button);
				}

			}
			g.window_end();
		}

		if (w.resize_level)
		{
			if (g.window_begin(editor::gui_names::resize_level))
			{
				// TODO:
				g.text("terrain stuff");
			}

			g.window_end();
		}
	}

	static std::vector<tile_position> get_tile_positions(level_editor_terrain::mouse_pos p,
		resources::tile_size_t tile_size, level_editor_terrain::draw_shape shape, int size)
	{
		const auto draw_pos = tile_position{
			static_cast<tile_position::value_type>(p.x / tile_size),
			static_cast<tile_position::value_type>(p.y / tile_size)
		};

		if (shape == level_editor_terrain::draw_shape::rect)
			return make_position_square(draw_pos, size);
		else if (shape == level_editor_terrain::draw_shape::circle)
			return make_position_circle(draw_pos, size);

		return std::vector<tile_position>{};
	}

	void level_editor_terrain::make_brush_preview(time_duration, mouse_pos p)
	{
		// NOTE: preview is ugly when drawing a terrain that doesn't have transitions
		//		this is common with the bottom layer terrain
		_preview = _clear_preview;

		const auto positions = get_tile_positions(p, _settings->tile_size, _shape, _size);

		if (_brush == brush_type::draw_terrain)
			_preview.place_terrain(positions, _current.terrain);
		else if (_brush == brush_type::draw_tile)
			_preview.place_tile(positions, _tile);

		//TODO: draw some kind of indicator for erasing using place tile
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

	void level_editor_terrain::on_click(mouse_pos p)
	{
		const auto positions = get_tile_positions(p, _settings->tile_size, _shape, _size);

		if (_brush == brush_type::draw_terrain)
			_map.place_terrain(positions, _current.terrain);
		else if (_brush == brush_type::draw_tile)
			_map.place_tile(positions, _tile);
		else if (_brush == brush_type::erase)
			_map.place_terrain(positions, _empty_terrain);
	}

	void level_editor_terrain::on_drag(mouse_pos p)
	{
		on_click(p);
	}

	void level_editor_terrain::draw(sf::RenderTarget &r, time_duration, sf::RenderStates s)
	{
		r.draw(_map, s);
	}

	void level_editor_terrain::draw_brush_preview(sf::RenderTarget &r, time_duration, sf::RenderStates s)
	{
		r.draw(_preview, s);
	}
}
