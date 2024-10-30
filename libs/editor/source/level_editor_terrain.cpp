#include "hades/level_editor_terrain.hpp"

#include "hades/camera.hpp"
#include "hades/gui.hpp"
#include "hades/level_editor.hpp"
#include "hades/properties.hpp"
#include "hades/terrain.hpp"

#include "hades/random.hpp" // temp; for generating heightmaps

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace hades
{
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

		// TODO: throw if terrainset == nullptr
		// remove check below

		if (_new_options.terrain_set &&
			_new_options.terrain == nullptr &&
			!empty(_new_options.terrain_set->terrains))
		{
			_new_options.terrain = _new_options.terrain_set->terrains.front().get();
		}

		//TODO: throw if terrain == nullptr
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
		
		constexpr auto variance = 3;
		const auto height_low = _settings->height_default - variance;
		const auto height_high = height_low + variance;

		const auto vert_size = get_vertex_size(map);
		for_each_position_rect({}, vert_size, vert_size, [&](const tile_position p) -> void {
			return change_terrain_height(p, map, *_settings, [&](const std::uint8_t) noexcept -> std::uint8_t {
				return integer_clamp_cast<std::uint8_t>(random(height_low, height_high));
				});
			});

		auto l = level{ std::move(current) };
		l.terrain = to_raw_terrain_map(std::move(map), *_settings);

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

		auto map = to_terrain_map(std::move(map_raw), *_settings);
		
		//TODO: if size != to level xy, then resize the map
		auto empty_map = make_map(size, _settings->editor_terrain ? _settings->editor_terrainset.get() : map.terrainset,
			_empty_terrain, *_settings);
		copy_heightmap(empty_map, map);

		_current.terrain_set = map.terrainset;
		_map.reset(std::move(map));
		_map.set_sun_angle(_sun_angle); // TODO: temp
		// _sun_angle = get_sun_angle(map);
		return;
	}

	level level_editor_terrain::level_save(level l) const
	{
		l.terrain = to_raw_terrain_map(_map.get_map(), *_settings);
		return l;
	}

	void level_editor_terrain::level_resize(vector2_int s, vector2_int o)
	{
		terrain_map map = _map.get_map(); // make copy of map
		const auto new_size_tiles = to_tiles(s, _tile_size);
		const auto offset_tiles = to_tiles(o, _tile_size);

		// TODO: ui to select new height
		resize_map(map, new_size_tiles, offset_tiles, _resize.terrain, _settings->height_default, *_settings);
		_map.reset(std::move(map));
		_resize.terrain = _empty_terrain;
	}

	// TODO: we should do this once when terrainset is set or changed and just store a vector of textures, strings and other data we need
	template<std::invocable<const resources::terrain*, const std::string&> Func>
	void make_terrain_button(gui& g, float tile_size_f, vector2_float button_size, const resources::terrain* terrain, Func&& func)
	{
		assert(terrain);
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
		g.push_id(terrain);
		const auto size = g.calculate_button_size({}, button_size);
		g.same_line_wrapping(size.x);
		const auto& str = data::get_as_string(terrain->id);
		if (g.image_button("###terrain_button"sv, *t.tex, tex_coords, button_size))
			std::invoke(func, terrain, str);
		g.tooltip(str);
		g.pop_id();
		return;
	};

	constexpr auto terrain_button_size = gui::vector2{
			25.f,
			25.f
	};

	void level_editor_terrain::gui_update(gui &g, editor_windows &w)
	{
		if (g.main_toolbar_begin())
		{			
			if (g.toolbar_button("terrain eraser"sv))
			{
				activate_brush();
				_terrain_palette.brush = brush_type::erase;
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

		const auto tile_size_f = float_cast(_tile_size);

		if (g.window_begin(editor::gui_names::toolbox))
		{
			// TODO: move to level properties?
			if (g.collapsing_header("sun settings"sv))
			{
				if (g.slider_scalar("Sun angle"sv, _sun_angle, 180.f, 0.f))
					_map.set_sun_angle(_sun_angle);
			}

			#if 0 // Deprecated menus
			if (g.collapsing_header("tile and terrain drawing settings"sv))
			{
				constexpr auto draw_shapes = std::array{
					"vertex"sv,
					"edge"sv,
					"square"sv,
					"circle"sv,
				};

				if (g.combo_begin("drawing shape"sv, draw_shapes[static_cast<std::size_t>(_terrain_palette.shape)]))
				{
					for (auto i = std::size_t{}; i < size(draw_shapes); ++i)
					{
						const auto shape = draw_shape{ integer_cast<std::underlying_type_t<draw_shape>>(i) };
						if (g.selectable(draw_shapes[i], _terrain_palette.shape == shape))
							_terrain_palette.shape = shape;
					}

					g.combo_end();
				}

				constexpr auto size_min = int{ 1 };
				constexpr auto size_max = int{ 8 };

				g.input_scalar("drawing size"sv, _size);
				_terrain_palette.draw_size = std::clamp(_size, size_min, size_max);
			}

			if (g.collapsing_header("height"sv))
			{
				g.slider_scalar("Amount"sv, _height_strength, std::uint8_t{ 1 }, std::uint8_t{ 10 });

				if (g.button("Raise"sv))
				{
					activate_brush();
					_terrain_palette.brush = brush_type::raise_terrain;
				}

				if (g.button("Lower"sv))
				{
					activate_brush();
					_terrain_palette.brush = brush_type::lower_terrain;
				}

				g.slider_scalar("Set Height"sv, _set_height, _settings->height_min, _settings->height_max);

				if (g.button("Set To"sv))
				{
					activate_brush();
					_terrain_palette.brush = brush_type::set_terrain_height;
				}
			}

			if (g.collapsing_header("cliffs"))
			{
				
				g.slider_scalar("Cliff height"sv, _cliff_default_height, _settings->cliff_height_min, std::uint8_t{ 20 });

				if (g.button("Create Cliff"sv))
				{
					activate_brush();
					_terrain_palette.brush = brush_type::raise_cliff;
				}
			}

			// TODO: hide this in release mode?
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
							const auto siz = g.calculate_button_size({}, terrain_button_size);
							g.same_line_wrapping(siz.x);
							if (g.image_button("###tile_button"sv, *t.tex, tex_coords, terrain_button_size))
							{
								activate_brush();
								_terrain_palette.brush = brush_type::draw_tile;
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
					_terrain_palette.brush = brush_type::draw_terrain;
					_current.terrain = _empty_terrain;
				}

				//if we have the empty terrainset then skip this,
				// the menu would be useless anyway
				if (_current.terrain_set != _empty_terrainset)
				{
					for (const auto& ter : _current.terrain_set->terrains)
					{
						make_terrain_button(g, tile_size_f, terrain_button_size, ter.get(), [&](auto&& t, const auto& str) {
							activate_brush();
							_terrain_palette.brush = brush_type::draw_terrain;
							_terrain_palette.brush_name = std::format("Terrain: {}", str);
							_current.terrain = t;
						});
					}
				}
			}
			#endif

			if (g.collapsing_header("terrain_palette"))
			{
				_gui_terrain_palette(g);
			}
		}
		g.window_end();

		if (w.new_level)
		{
			if (g.window_begin(editor::gui_names::new_level))
			{;
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
					// TODO: terrain_string_label, follow the same structure as the terrain palette titles
					//		we can do this lookup once and store the title
					// We should do this with all the new/resize dialog boxes
					g.text(data::get_as_string(_new_options.terrain->id));

					if (g.button("empty"sv))
						_new_options.terrain = _empty_terrain;

					for (const auto& ter : _new_options.terrain_set->terrains)
					{
						make_terrain_button(g, tile_size_f, terrain_button_size, ter.get(), [&](auto&& t, auto&& /*unused*/) noexcept {
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
					make_terrain_button(g, tile_size_f, terrain_button_size, ter.get(), [&](auto&& t, auto&& /*unused*/) noexcept {
						_resize.terrain = t;
						});
				}
			}

			g.window_end();
		}
	}

	template<typename Func>
	concept invoke_vertex = std::invocable<Func, const tile_position, const rect_corners, const bool /*left_triangle*/>;

	template<typename Func>
	concept invoke_edge = always_true_v<Func>;

	struct vertex_tag_t {};
	constexpr auto vertex_tag = vertex_tag_t{};

	// optionally std::invocable<Func, triangle_info> and or std::invocable<Func, tile_position, float> as well
	// these support inter cell targets and fading strength circle shapes
	template<typename Func> // Effectively invoke_tile
	concept invoke_position = std::invocable<Func, tile_position> && 
		invocable_for_each_circle<Func> &&
		std::invocable<Func, vertex_tag_t, terrain_vertex_position>;

	template<invoke_position Func>
	static void for_each_safe_diag(terrain_vertex_position position, const terrain_vertex_position diff,
		int distance, const terrain_vertex_position world_size, Func&& f)
	{
		while (distance-- > 0)
		{
			std::invoke(f, position);
			position += diff;

			if (!within_world(position, world_size))
				break;
		}

		return;
	}

	template<invoke_position Func>
	static void do_diag_edge(terrain_vertex_position vert, vector2_float frac_pos,
		const terrain_map::triangle_type triangle_type, const int size, const terrain_vertex_position world_size, Func&& f)
	{
		if (frac_pos.x >= .5f)
			++vert.x;
		if (frac_pos.y >= .5f)
			++vert.y;

		auto diff = terrain_vertex_position{ 1, 1 };
		const auto half = size / 2;
		vert.x -= half;
		if (triangle_type == terrain_map::triangle_uphill)
		{
			vert.y += half;
			diff.y = -1;
		}
		else
			vert.y -= half;

		// start
		// length
		// direction
		return for_each_safe_diag(vert, diff, size + 1, world_size, std::forward<Func>(f));
	}

	constexpr auto downhill_line = line_t<float>{ {0.f, 0.f}, { 1.f, 1.f } };
	constexpr auto uphill_line = line_t<float>{ { 0.f, 1.f }, { 1.f, 0.f } };

	using brush_t = level_editor_terrain::brush_type;
	using brush_shape_t = level_editor_terrain::draw_shape;
	template<invoke_position Func>
	static void for_each_position(const level_editor_terrain::mouse_pos p,
		const resources::tile_size_t tile_size, const level_editor_terrain::draw_shape shape,
		const brush_t brush, int size, const terrain_map& map,
		Func&& f) //		  ^^^^^ TODO: make this uint16_t
	{
		const auto world_size = get_size(map);
		const auto world_vertex_size = world_size + tile_position{ 1, 1 };

		const auto draw_pos_f = world_vector_t{
			p.x / float_cast(tile_size),
			p.y / float_cast(tile_size)
		};

		const auto trunc_pos = world_vector_t{
				std::floor(draw_pos_f.x),
				std::floor(draw_pos_f.y)
		};

		// tile_pos
		auto tile_pos = static_cast<terrain_vertex_position>(trunc_pos);

		if (!within_world(tile_pos, world_size))
			return;

		const auto frac_pos = draw_pos_f - trunc_pos;

		auto vertex = static_cast<terrain_vertex_position>(world_vector_t{
				std::round(draw_pos_f.x),
				std::round(draw_pos_f.y)
		});

		auto vertex_target = true;
		switch (brush)
		{
		case brush_t::raise_cliff:
			[[fallthrough]];
		case brush_t::lower_cliff:
			[[fallthrough]];
		case brush_t::raise_water:
			[[fallthrough]];
		case brush_t::lower_water:
			vertex_target = false;
		}

		using draw_shape = level_editor_terrain::draw_shape;
		switch (shape)
		{
		case draw_shape::rect:
		{
			auto rect_size = terrain_vertex_position{ 1, 1 };
			switch (size)
			{
				// NOTE: starting values are already correct for case 1
			case 2:
				vertex -= terrain_vertex_position{ 1, 1 };
				tile_pos -= terrain_vertex_position{ 1, 1 };
				rect_size *= 3;
				break;
			case 3:
				vertex -= terrain_vertex_position{ 2, 2 };
				tile_pos -= terrain_vertex_position{ 2, 2 };
				rect_size *= 5;
				break;
			case 5:
				vertex -= terrain_vertex_position{ 4, 4 };
				tile_pos -= terrain_vertex_position{ 4, 4 };
				rect_size *= 9;
				break;
			case 8:
				vertex -= terrain_vertex_position{ 7, 7 };
				tile_pos -= terrain_vertex_position{ 7, 7 };
				rect_size *= 15;
				break;
			}

			if (vertex_target)
			{
				return for_each_safe_position_rect(vertex, rect_size, world_size + tile_position{ 1, 1 }, [&f](auto&& pos) {
					std::invoke(f, vertex_tag, pos);
					});
			}
			else
				return for_each_safe_position_rect(tile_pos, rect_size, world_size, std::forward<Func>(f));
		}
		case draw_shape::circle:
		{
			auto smooth_strength_circle = false;
			switch (brush)
			{
			case brush_t::raise_terrain:
				[[fallthrough]];
			case brush_t::lower_terrain:
				smooth_strength_circle = true;
			}

			if (vertex_target)
			{
				auto vert_func = [&f](auto&& pos) {
					std::invoke(f, vertex_tag, pos);
				};

				if (smooth_strength_circle)
				{
					if constexpr (std::invocable<Func, terrain_vertex_position, float>)
					{
						auto strength_func = [f](const terrain_vertex_position pos, const float strength) {
							std::invoke(f, pos, strength);
							};

						return for_each_safe_position_circle(vertex, size, world_size + tile_position{ 1, 1 }, overloaded{ vert_func, strength_func });
					}
				}
				return for_each_safe_position_circle(vertex, size - 1, world_size + tile_position{ 1, 1 }, vert_func);
			}
			return for_each_safe_position_circle(tile_pos, size - 1, world_size, std::forward<Func>(f));
		}
		}

		return;
	}

	void level_editor_terrain::make_brush_preview(const time_duration, const mouse_pos p)
	{
		const auto tile_func = [&](const tile_position pos) {
			_map.set_edit_target_style(mutable_terrain_map::edit_target::tile);
			_map.add_edit_target(pos);
		};

		const auto vertex_func = [&](const vertex_tag_t, const terrain_vertex_position pos) {
			_map.set_edit_target_style(mutable_terrain_map::edit_target::vertex);
			_map.add_edit_target(pos);
		};

		_map.clear_edit_target();
		for_each_position(p, _settings->tile_size, _terrain_palette.shape,
			_terrain_palette.brush, _terrain_palette.draw_size,	_map.get_map(),
			overloaded{ tile_func, vertex_func });
		
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

		const auto& map = _map.get_map();
		const auto world_size = get_size(map);
		auto out = tag_list{};
		for_each_position_rect({ loc.x, loc.y }, { loc.width, loc.height }, world_size, [&map, &out](const tile_position p) {
			const auto tags = hades::get_tags_at(map, p);
			out.insert(end(out), begin(tags), end(tags));
		});

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
		// called for cliff layer tools
		const auto tile_func = [&](const tile_position) {
			switch (_terrain_palette.brush)
			{
			case brush_type::raise_cliff:
				break;
			case brush_type::lower_cliff:
				break;
			/*case brush_type::raise_water:
				break;
			case brush_type::lower_water:
				break;*/
			}	
		};

		// called for static strength vertex targeting tools (terrain drawing, usually)
		const auto vertex_func = [&](const vertex_tag_t, const terrain_vertex_position pos) {
			switch (_terrain_palette.brush)
			{
			case brush_type::draw_terrain:
				_map.place_terrain(pos, _current.terrain);
				break;
			/*case brush_type::erase: // a special case of terrain drawing
					_map.place_terrain(pos, _empty_terrain);
				break;*/
			}
			return;
		};

		// NOTE: strength is only used for heightmap editing which targets vertex,
		//			no need for a tile_func_strength
		const auto vertex_func_strength = [&](const terrain_vertex_position pos, float strength) {
			switch (_terrain_palette.brush)
			{
			default:
				return vertex_func(vertex_tag, pos);
			case brush_type::raise_terrain:
			{
				const auto str = integral_cast<std::uint8_t>(_terrain_palette.draw_size * strength);
				_map.raise_terrain(pos, str);
			}break;
			case brush_type::lower_terrain:
			{
				const auto str = integral_cast<std::uint8_t>(_terrain_palette.draw_size * strength);
				_map.lower_terrain(pos, str);
			}break;
			}
			return;
		};

		_map.clear_edit_target();

		const auto ovrld = overloaded{ tile_func, vertex_func, vertex_func_strength };
		for_each_position(p, _settings->tile_size, _terrain_palette.shape, _terrain_palette.brush, _terrain_palette.draw_size, _map.get_map(), ovrld);
		return;
	}

	void level_editor_terrain::on_drag(mouse_pos p)
	{
		// TODO: need custom code here for plateaus
		on_click(p);
	}

	void level_editor_terrain::on_screen_move(rect_float r)
	{
		_map.set_world_region(r);
		return;
	}

	void level_editor_terrain::on_height_toggle(bool b) noexcept
	{
		_map.set_height_enabled(b);
		return;
	}

	void level_editor_terrain::draw(sf::RenderTarget &r, time_duration, sf::RenderStates s)
	{
		if (!is_active_brush())
			_map.clear_edit_target();
		_map.apply();
		_map.set_world_rotation(get_world_rotation());
		r.draw(_map, s);
	}

	namespace detail
	{
		struct size_selection
		{
			std::string_view button_label;
			std::string_view title_label;
			uint8_t value;
		};

		constexpr auto size_selections = std::array<size_selection, 5>{
			size_selection{ "1"sv, "Size: 1"sv, 1 }, 
			size_selection{ "2"sv, "Size: 2"sv, 2 },
			size_selection{ "3"sv, "Size: 3"sv, 3 },
			size_selection{ "5"sv, "Size: 5"sv, 5 },
			size_selection{ "8"sv, "Size: 8"sv, 8 },
		};

		struct brush_selection
		{
			std::string_view button_label;
			std::string_view title_label;
			brush_t brush;
		};

		constexpr auto height_selections = std::array<brush_selection, 5>{
			brush_selection{ "Raise"sv,		"Height: Raise"sv,		brush_t::raise_terrain },
			brush_selection{ "Lower"sv,		"Height: Lower"sv,		brush_t::lower_terrain },
			brush_selection{ "Plateau"sv,	"Height: Plateau"sv,	brush_t::plateau_terrain },
			brush_selection{ "Noise"sv,		"Height: Noise"sv,		brush_t::add_height_noise },
			brush_selection{ "Smooth"sv,	"Height: Smooth"sv,		brush_t::smooth_height },
		};

		constexpr auto cliff_selections = std::array<brush_selection, 4>{
			brush_selection{ "Raise"sv,		"Cliff: Raise"sv,	brush_t::raise_cliff },
			brush_selection{ "Lower"sv,		"Cliff: Lower"sv,	brush_t::lower_cliff },
			brush_selection{ "Plateau"sv,	"Cliff: Plateau"sv,	brush_t::plataeu_cliff },
			brush_selection{ "Ramp"sv,		"Cliff: Ramp"sv,	brush_t::add_ramp }
		};

		constexpr auto water_selections = std::array<brush_selection, 2>{
			brush_selection{ "Raise"sv,		"Water: Raise"sv,	brush_t::raise_water },
			brush_selection{ "Lower"sv,		"Water: Lower"sv,	brush_t::lower_water }
		};

		struct shape_selection
		{
			std::string_view button_label;
			std::string_view title_label;
			brush_shape_t shape;
		};

		constexpr auto shape_selections = std::array<shape_selection, 2>{
			shape_selection{ "Circle"sv, "Shape: Circle"sv,	brush_shape_t::circle },
			shape_selection{ "Square"sv, "Shape: Square"sv,	brush_shape_t::rect }
		};
	}

	void level_editor_terrain::_gui_terrain_palette(gui& g)
	{
		// Terrain Tools
		g.radio_button("###terrain_activated"sv, _terrain_palette.brush == brush_type::draw_terrain);
		g.same_line();
		if (_terrain_palette.brush == brush_type::draw_terrain)
			g.separator_text(_terrain_palette.brush_name);
		else
			g.separator_text("Terrain"sv);
		g.dummy();
		// if we have the empty terrainset then skip this,
		// the menu would be useless anyway
		if (_current.terrain_set != _empty_terrainset)
		{
			const auto tile_size_f = float_cast(_tile_size);
			for (const auto& ter : _current.terrain_set->terrains)
			{
				make_terrain_button(g, tile_size_f, terrain_button_size, ter.get(), [&](auto&& t, const auto& str) {
					activate_brush();
					_terrain_palette.brush = brush_type::draw_terrain;
					_terrain_palette.brush_name = std::format("Terrain: {}"sv, str);
					_current.terrain = t;
					});
			}
		}
		g.separator_horizontal();

		const auto brush_selector = [&](const std::span<const detail::brush_selection> selections) {
			g.dummy();
			for (const auto& select : selections)
			{
				const auto b_size = g.calculate_button_size(select.button_label);
				g.same_line_wrapping(b_size.x);
				g.push_id(enum_type(select.brush));
				if (g.button(select.button_label))
				{
					_terrain_palette.brush_name = select.title_label;
					_terrain_palette.brush = select.brush;
					activate_brush();
				}
				g.pop_id();
			}
		};

		// Cliff Tools
		const auto cliff_mode = _terrain_palette.brush == brush_type::raise_cliff ||
			_terrain_palette.brush == brush_type::lower_cliff ||
			_terrain_palette.brush == brush_type::plataeu_cliff ||
			_terrain_palette.brush == brush_type::add_ramp;
		g.radio_button("###cliff_activated"sv, cliff_mode);
		g.same_line();
		if (cliff_mode)
			g.separator_text(_terrain_palette.brush_name);
		else
			g.separator_text("Cliffs"sv);
		brush_selector(detail::cliff_selections);
		g.separator_horizontal();

		// Height Tools
		const auto height_mode = _terrain_palette.brush == brush_type::raise_terrain ||
			_terrain_palette.brush == brush_type::lower_terrain ||
			_terrain_palette.brush == brush_type::plateau_terrain ||
			_terrain_palette.brush == brush_type::smooth_height ||
			_terrain_palette.brush == brush_type::add_height_noise;
		g.radio_button("###height_activated"sv, height_mode);
		g.same_line();
		if (height_mode)
			g.separator_text(_terrain_palette.brush_name);
		else
			g.separator_text("Height"sv);
		brush_selector(detail::height_selections);
		g.separator_horizontal();

#if 1
		// Water Tool
		const auto water_mode = _terrain_palette.brush == brush_type::raise_water ||
			_terrain_palette.brush == brush_type::lower_water;
		g.radio_button("####water_activated", water_mode);
		g.same_line();
		if (water_mode)
			g.separator_text(_terrain_palette.brush_name);
		else
			g.separator_text("Water"sv);
		brush_selector(detail::water_selections);
		g.separator_horizontal();
#endif

		// Size Selector
		g.text(_terrain_palette.size_label);
		g.dummy();
		for (const auto& select : detail::size_selections)
		{
			const auto b_size = g.calculate_button_size(select.button_label);
			g.same_line_wrapping(b_size.x);
			if (g.button(select.button_label))
			{
				_terrain_palette.size_label = select.title_label;
				_terrain_palette.draw_size = select.value;
			}
		}

		// Drawing Shape
		g.text(_terrain_palette.shape_label);
		g.dummy();
		for (const auto& select : detail::shape_selections)
		{
			const auto b_size = g.calculate_button_size(select.button_label);
			g.same_line_wrapping(b_size.x);
			if (g.button(select.button_label))
			{
				_terrain_palette.shape_label = select.title_label;
				_terrain_palette.shape = select.shape;
			}
		}

		return;
	}
}
