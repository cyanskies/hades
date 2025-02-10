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
		constexpr auto perlin_scale = 0.5f;
		const auto height_low = _settings->height_default - variance;
		const auto height_high = height_low + variance;

		const auto vert_size = get_vertex_size(map);
		for_each_position_rect({}, vert_size, vert_size, [&](const tile_position p) -> void {
			return change_terrain_height(p, map, *_settings, [&](const std::uint8_t h) noexcept -> std::uint8_t {
				return integral_cast<std::uint8_t>(h + perlin_noise(float_cast(p.x) * perlin_scale, float_cast(p.y) * perlin_scale) * variance);
				});
			});

		auto l = level{ std::move(current) };
		l.terrain = to_raw_terrain_map(std::move(map), *_settings);

		return l;
	}

	void level_editor_terrain::level_load(const level &l)
	{
		_store_map();
		auto map_raw = l.terrain;

		//check the scale and size of the map
		assert(_settings);
		const auto tile_size = _settings->tile_size;

		const auto size = tile_position{
			signed_cast(l.map_x / tile_size),
			signed_cast(l.map_y / tile_size)
		};

		assert(size.x >= 0 && size.y >= 0);

		//if terrainset is empty then assign one
		if (map_raw.terrainset == unique_id::zero)
		{
			if (std::empty(_settings->terrainsets))
				map_raw.terrainset = _settings->empty_terrainset->id;
			else
				map_raw.terrainset = _settings->terrainsets.front()->id;
		}

		auto t_map = to_terrain_map(std::move(map_raw), *_settings);
		
		_current.terrain_set = t_map.terrainset;
		auto& map = get_map();
		map.reset(std::move(t_map));
		map.set_sun_angle(_sun_angle); // TODO: temp
		// _sun_angle = get_sun_angle(map);
		return;
	}

	level level_editor_terrain::level_save(level l) const
	{
		l.terrain = to_raw_terrain_map(_map->get_map(), *_settings);
		return l;
	}

	void level_editor_terrain::level_resize(vector2_int s, vector2_int o)
	{
		terrain_map map = _map->get_map(); // make copy of map
		const auto new_size_tiles = to_tiles(s, _tile_size);
		const auto offset_tiles = to_tiles(o, _tile_size);

		// TODO: ui to select new height
		resize_map(map, new_size_tiles, offset_tiles, _resize.terrain, _settings->height_default, *_settings);
		_map->reset(std::move(map));
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
		if (g.main_menubar_begin())
		{
			if (g.menu_begin("view"sv))
			{
				if (auto ramps = _map->show_ramps(); g.menu_toggle_item("show ramp indicators"sv, ramps))
					_map->show_ramps(ramps);
				if (auto cliffs = _map->show_cliff_edges(); g.menu_toggle_item("show cliff indicators"sv, cliffs))
					_map->show_cliff_edges(cliffs);
				if (auto layers = _map->show_cliff_layers(); g.menu_toggle_item("show cliff layer value"sv, layers))
					_map->show_cliff_layers(layers);
				if (auto shadows = _map->show_shadows(); g.menu_toggle_item("draw shadows"sv, shadows))
					_map->show_shadows(shadows);
				if (auto depth = _map->draw_depth_buffer(); g.menu_toggle_item("draw depth buffer"sv, depth))
					_map->draw_depth_buffer(depth);
				if (auto normals = _map->draw_normals_buffer(); g.menu_toggle_item("draw normal buffer"sv, normals))
					_map->draw_normals_buffer(normals);

				g.menu_end();
			}
			
			g.main_menubar_end();
		}

		if (g.main_toolbar_begin())
		{
			if (g.toolbar_button("debug tool"sv))
			{
				activate_brush();
				_terrain_palette.brush = brush_type::debug_brush;
			}

			if (g.toolbar_button("terrain eraser"sv))
			{
				activate_brush();
				_terrain_palette.brush = brush_type::erase;
			}

			// TODO: move both of these into the view menu
			if (g.toolbar_button("terrain grid"sv))
			{
				_map->show_grid(!_map->show_grid());
			}

			if (g.toolbar_button("terrain depth"sv))
			{
				_map->draw_depth_buffer(!_map->draw_depth_buffer());
			}
		}

		g.main_toolbar_end();

		const auto tile_size_f = float_cast(_tile_size);

		if (g.window_begin(editor::gui_names::toolbox))
		{
			// TODO: move to level properties?
			if (g.collapsing_header_begin("sun settings"sv))
			{
				if (g.slider_scalar("Sun angle"sv, _sun_angle, 180.f, 0.f))
					_map->set_sun_angle(_sun_angle);
				g.collapsing_header_end();
			}

			if (g.collapsing_header_begin("terrain_palette"))
			{
				_gui_terrain_palette(g);
				g.collapsing_header_end();
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

	static terrain_vertex_position to_vertex_pos(const world_vector_t pixel_target, const float tile_size) noexcept
	{
		const auto draw_pos_f = world_vector_t{
			pixel_target.x / tile_size,
			pixel_target.y / tile_size
		};

		return static_cast<terrain_vertex_position>(world_vector_t{
			std::round(draw_pos_f.x),
			std::round(draw_pos_f.y)
		});
	}

	struct vertex_tag_t {};
	constexpr auto vertex_tag = vertex_tag_t{};

	// optionally std::invocable<Func, triangle_info> and or std::invocable<Func, tile_position, float> as well
	// these support inter cell targets and fading strength circle shapes
	template<typename Func> // Effectively invoke_tile
	concept invoke_position = std::invocable<Func, tile_position> && 
		invocable_for_each_circle<Func> &&
		std::invocable<Func, vertex_tag_t, terrain_vertex_position>;

	using brush_t = level_editor_terrain::brush_type;
	using brush_shape_t = level_editor_terrain::draw_shape;
	template<invoke_position Func>
	static void for_each_position(const terrain_target t,
		const resources::tile_size_t tile_size, const level_editor_terrain::draw_shape shape,
		const brush_t brush, int size, const terrain_map& map,
		Func&& f) //		  ^^^^^ TODO: make this uint16_t
	{
		const auto world_size = get_size(map);
		const auto world_vertex_size = world_size + tile_position{ 1, 1 };

		// tile_pos
		auto tile_pos = t.tile_target;

		if (!within_world(tile_pos, world_size))
			return;

		auto vertex = to_vertex_pos(t.pixel_target, float_cast(tile_size));

		auto vertex_target = true;
		switch (brush)
		{
		case brush_t::raise_cliff:
			[[fallthrough]];
		case brush_t::lower_cliff:
			[[fallthrough]];
		case brush_t::plataeu_cliff:
			[[fallthrough]];
		case brush_t::add_ramp:
			[[fallthrough]];
		case brush_t::remove_ramp:
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
				[[fallthrough]];
			case brush_t::add_height_noise:
				[[fallthrough]];
			case brush_t::smooth_height:
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

			const auto tile_func = [&f](const tile_position p) {
				std::invoke(f, p);
				};

			return for_each_safe_position_circle(tile_pos, size - 1, world_size, tile_func);
		}
		}

		return;
	}

	void level_editor_terrain::make_brush_preview(const time_duration, const std::optional<terrain_target> t)
	{
		// skip generating the preview if the inputs from the previous gen haven't changed
		if (const auto preview = std::tuple{ _terrain_palette.brush, _terrain_palette.shape, _terrain_palette.draw_size, t };
			preview == _terrain_palette.last_preview)
			return;
		else
			_terrain_palette.last_preview = preview;

		const auto tile_func = [&](const tile_position pos) {
			_map->set_edit_target_style(mutable_terrain_map::edit_target::tile);
			switch (_terrain_palette.brush)
			{
			case brush_type::add_ramp:
				if (can_add_ramp(pos, _map->get_map()).any())
					_map->add_edit_target(pos);
				break;
			case brush_type::remove_ramp:
				{
					if (get_ramps(pos, _map->get_map()).any())
						_map->add_edit_target(pos);
				} break;
			default:
				_map->add_edit_target(pos);
			}
		};

		const auto vertex_func = [&](const vertex_tag_t, const terrain_vertex_position pos) {
			_map->set_edit_target_style(mutable_terrain_map::edit_target::vertex);
			_map->add_edit_target(pos);
		};

		_map->clear_edit_target();
		if (t)
		{
			for_each_position(*t, _settings->tile_size, _terrain_palette.shape,
				_terrain_palette.brush, _terrain_palette.draw_size, _map->get_map(),
				overloaded{ tile_func, vertex_func });
		}
		return;
	}

	void level_editor_terrain::on_reinit(vector2_float window_size, vector2_float view_size)
	{
		auto view_height = static_cast<float>(*_view_height) * editor::zoom_max;
		auto default_w = camera::calculate_width(static_cast<float>(*_view_height), window_size.x, window_size.y);
		constexpr auto chunk_count = 9;
		const auto chunk_size = std::max(view_height, default_w * editor::zoom_max) / 3.f;
		_map->set_chunk_size(integral_cast<std::size_t>(chunk_size, round_up_tag));

		//also do screen move
	}

	void level_editor_terrain::on_click(std::optional<terrain_target> p)
	{
		if (_terrain_palette.brush == brush_type::debug_brush)
			return;

		_map->clear_edit_target();

		if (!p)
			return;

		// Calculate average height under the brush if we're using
		//	the smoothing tool
		std::uint8_t avrg_height [[indeterminate]];
		if (_terrain_palette.brush == brush_type::smooth_height)
		{
			auto height_sum = int{};
			auto height_count = int{};
			const auto sample_stub1 = [](const tile_position) {};
			const auto sample_stub2 = [](const vertex_tag_t, const terrain_vertex_position) {};
			const auto sample_collector = [&](const terrain_vertex_position pos, const float) {
				using h_type = terrain_map::vertex_height_t;
				using limits = std::numeric_limits<h_type>;
				height_sum += get_vertex_height(pos, _map->get_map());
				++height_count;
			};

			const auto overload = overloaded{ sample_stub1, sample_stub2, sample_collector };
			for_each_position(*p, _settings->tile_size, _terrain_palette.shape, _terrain_palette.brush, _terrain_palette.draw_size, _map->get_map(), overload);
			avrg_height = integer_clamp_cast<std::uint8_t>(height_sum / height_count);
		}

		// called for cliff layer tools
		const auto tile_func = [&](const tile_position p) {
			switch (_terrain_palette.brush)
			{
			case brush_type::raise_cliff:
				_map->raise_cliff(p);
				break;
			case brush_type::lower_cliff:
				_map->lower_cliff(p);
				break;
			case brush_type::add_ramp:
				_map->place_ramp(p);
				break;
			case brush_type::remove_ramp:
				_map->remove_ramp(p);
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
				_map->place_terrain(pos, _current.terrain);
				break;
			/*case brush_type::erase: // a special case of terrain drawing
					_map->place_terrain(pos, _empty_terrain);
				break;*/
			}
			return;
		};

		// NOTE: strength is only used for heightmap editing which targets vertex,
		//			no need for a tile_func_strength
		const auto vertex_func_strength = [&](const terrain_vertex_position pos, const float strength) {
			using h_type = terrain_map::vertex_height_t;
			using limits = std::numeric_limits<h_type>;
			switch (_terrain_palette.brush)
			{
			case brush_type::raise_terrain:
			{
				const auto str = integral_cast<h_type>(_terrain_palette.draw_size * strength);
				_map->raise_terrain(pos, str);
			}break;
			case brush_type::lower_terrain:
			{
				const auto str = integral_cast<h_type>(_terrain_palette.draw_size * strength);
				_map->lower_terrain(pos, str);
			}break;
			case brush_type::add_height_noise:
			{
				const auto h = get_vertex_height(pos, _map->get_map());
				const auto noise = perlin_noise(float_cast(pos.x) * 0.75f, float_cast(pos.y) * 0.75f);
				const auto diff = noise * strength * _terrain_palette.draw_size;
				const auto new_h_f = std::clamp(h + diff, float_cast(limits::min()), float_cast(limits::max()));
				const auto new_h = integral_cast<std::uint8_t>(new_h_f);
				_map->set_terrain_height(pos, new_h);
			}break;
			case brush_type::smooth_height:
			{
				const auto h = get_vertex_height(pos, _map->get_map());
				const auto diff = avrg_height - h;
				const auto str = integral_cast<h_type>(_terrain_palette.draw_size * strength);
				if (diff < 0)
				{
					if (std::cmp_less(h - str, avrg_height))
						_map->set_terrain_height(pos, avrg_height);
					else
						_map->lower_terrain(pos, str);
				}
				else
				{
					if (std::cmp_greater(h + str, avrg_height))
						_map->set_terrain_height(pos, avrg_height);
					else
						_map->raise_terrain(pos, str);
				}
			}break;

			}
			return;
		};

		const auto ovrld = overloaded{ tile_func, vertex_func, vertex_func_strength };
		for_each_position(*p, _settings->tile_size, _terrain_palette.shape, _terrain_palette.brush, _terrain_palette.draw_size, _map->get_map(), ovrld);
		return;
	}

	struct on_drag_functor
	{
		using brush_type = level_editor_terrain::brush_type;

		mutable_terrain_map& map;
		std::uint8_t stored_height;
		brush_type brush;

		void operator()(const tile_position p)
		{
			switch (brush)
			{
			case brush_type::raise_cliff:
				[[fallthrough]];
			case brush_type::lower_cliff:
				[[fallthrough]];
			case brush_type::plataeu_cliff:
				map.set_cliff(p, stored_height);
				break;
			default:
				log_error("Wrong brush passed to on_drag_functor(tile_position)"sv);
			}

			return;
		}

		void operator()(const vertex_tag_t, const terrain_vertex_position pos)
		{
			switch (brush)
			{
			case brush_type::plateau_terrain:
				map.set_terrain_height(pos, stored_height);
				break;
			default:
				log_error("Wrong brush passed to on_drag_functor(terrain_vertex_position)"sv);
			}

			return;
		}
	};

	static std::uint8_t pick_next_height(const terrain_map& map,
		const level_editor_terrain::brush_type brush, const terrain_target& t,
		const resources::terrain_settings& s)
	{
		using enum level_editor_terrain::brush_type;
		switch (brush)
		{
		case raise_cliff:
		{
			const auto h = get_cliff_layer(t.tile_target, map);
			const auto next_h = h + 1;
			const auto safe_h = std::clamp(next_h, integer_cast<decltype(next_h)>(s.cliff_min), integer_cast<decltype(next_h)>(s.cliff_max));
			return integer_cast<std::uint8_t>(safe_h);
		}
		case lower_cliff:
		{
			const auto h = get_cliff_layer(t.tile_target, map);
			const auto next_h = h - 1;
			const auto safe_h = std::clamp(next_h, integer_cast<decltype(next_h)>(s.cliff_min), integer_cast<decltype(next_h)>(s.cliff_max));
			return integer_cast<std::uint8_t>(safe_h);
		}
		case plateau_terrain:
			return get_vertex_height(to_vertex_pos(t.pixel_target, float_cast(s.tile_size)), map);
		case plataeu_cliff:
			return get_cliff_layer(t.tile_target, map);;
		}

		throw level_editor_error{ "Failed to find map height while dragging"s };
	}

	void level_editor_terrain::on_drag_start(const std::optional<terrain_target> t)
	{
		if (!t)
		{
			_terrain_palette.drag_level.reset();
			return;
		}

		if (_no_drag(t))
			return;

		_map->clear_edit_target();
		const auto h = pick_next_height(_map->get_map(), _terrain_palette.brush, *t, *_settings);
		_terrain_palette.drag_level = h;
		auto functor = on_drag_functor{ *_map, h, _terrain_palette.brush };
		for_each_position(*t, _settings->tile_size, _terrain_palette.shape, _terrain_palette.brush, _terrain_palette.draw_size, _map->get_map(), functor);
		return;
	}

	void level_editor_terrain::on_drag(const std::optional<terrain_target> t)
	{
		if (!t)
			return;
		if (_no_drag(t))
			return;

		_map->clear_edit_target();

		if(!_terrain_palette.drag_level)
		{
			const auto h = pick_next_height(_map->get_map(), _terrain_palette.brush, *t, *_settings);
			_terrain_palette.drag_level = h;
		}
		auto functor = on_drag_functor{ *_map, _terrain_palette.drag_level.value(), _terrain_palette.brush};
		for_each_position(*t, _settings->tile_size, _terrain_palette.shape, _terrain_palette.brush, _terrain_palette.draw_size, _map->get_map(), functor);
		return;
	}

	void level_editor_terrain::on_screen_move(rect_float r)
	{
		_map->set_world_region(r);
		return;
	}

	void level_editor_terrain::on_height_toggle(bool b) noexcept
	{
		_map->set_height_enabled(b);
		return;
	}

	void level_editor_terrain::draw(sf::RenderTarget &r, time_duration, sf::RenderStates s)
	{
		auto& map = get_map();
		if (!is_active_brush())
			map.clear_edit_target();
		map.apply();
		map.set_world_rotation(get_world_rotation());
		r.draw(map, s);
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

		constexpr auto cliff_selections = std::array<brush_selection, 5>{
			brush_selection{ "Raise"sv,			"Cliff: Raise"sv,		brush_t::raise_cliff },
			brush_selection{ "Lower"sv,			"Cliff: Lower"sv,		brush_t::lower_cliff },
			brush_selection{ "Plateau"sv,		"Cliff: Plateau"sv,		brush_t::plataeu_cliff },
			brush_selection{ "Ramp"sv,			"Cliff: Ramp"sv,		brush_t::add_ramp },
			brush_selection{ "Remove Ramp"sv,	"Cliff: Remove Ramp"sv, brush_t::remove_ramp }
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
			_terrain_palette.brush == brush_type::add_ramp ||
			_terrain_palette.brush == brush_type::remove_ramp;
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

#if 0
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

	bool level_editor_terrain::_no_drag(std::optional<terrain_target> t)
	{
		using enum brush_type;
		switch (_terrain_palette.brush)
		{
		case draw_terrain:
			[[fallthrough]];
		case raise_terrain:
			[[fallthrough]];
		case lower_terrain:
			[[fallthrough]];
		case add_ramp:
			[[fallthrough]];
		case remove_ramp:
			[[fallthrough]];
		case add_height_noise:
			[[fallthrough]];
		case smooth_height:
			[[fallthrough]];
		case debug_brush:
			on_click(t);
			return true;
		}
		return false;
	}
}
