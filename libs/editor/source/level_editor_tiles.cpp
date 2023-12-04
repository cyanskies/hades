#include "hades/level_editor_tiles.hpp"

#include "hades/gui.hpp"
#include "hades/mouse_input.hpp"
#include "hades/tiles.hpp"

using namespace std::string_view_literals;

namespace hades
{
	void register_level_editor_tiles_resources(data::data_manager &d)
	{
		register_tile_map_resources(d);
	}

	level_editor_tiles::level_editor_tiles()
		: _settings{ resources::get_tile_settings() }
	{
		// load all tilesets
		for (auto& t : _settings->tilesets)
		{
			data::get<resources::tileset>(t.id());
		}
	}

	level level_editor_tiles::level_new(level l) const
	{
		const auto tile_size = _settings->tile_size;
		const auto size = vector2_int{
			signed_cast(l.map_x / tile_size),
			signed_cast(l.map_y / tile_size)
		};

		if (size.x * tile_size != l.map_x ||
			size.y * tile_size != l.map_y)
			LOGWARNING("new map size must be a multiple of tile size: [" +
				to_string(tile_size) + "], will be adjusted to the a valid value");

		const auto &tile = 
			_new_options.tile ? *_new_options.tile : resources::get_empty_tile();

		const auto map = make_map(size, tile);
		l.tile_map_layer = to_raw_map(map);

		return l;
	}

	void level_editor_tiles::level_load(const level &l)
	{
		_level_size = { l.map_x, l.map_y };

		assert(_settings);
		const auto tile_size = _settings->tile_size;
		const auto size = tile_position{
			signed_cast(l.map_x / tile_size),
			signed_cast(l.map_y / tile_size)
		};

		if (size.x * tile_size != l.map_x ||
			size.y * tile_size != l.map_y)
			LOGWARNING("loaded map size must be a multiple of tile size: [" + 
				to_string(tile_size) + "], level will be adjusted to a valid value");

		const auto empty_map = make_map(size, resources::get_empty_tile());
		_empty_preview.create(empty_map);

		if (l.tile_map_layer.tiles.empty())
			_tiles.create(empty_map);
		else
		{
			//TODO: if size != to level xy, then resize the map
			const auto map = to_tile_map(l.tile_map_layer);
			_tiles.create(map);
		}
	}

	level level_editor_tiles::level_save(level l) const
	{
		const auto map = _tiles.get_map();
		l.tile_map_layer = to_raw_map(map);
		return l;
	}

	void level_editor_tiles::level_resize(vector2_int s, vector2_int o)
	{
		auto map = _tiles.get_map();
		const auto new_size_tiles = to_tiles(s, _settings->tile_size);
		const auto offset_tiles = to_tiles(o, _settings->tile_size);
		_level_size = new_size_tiles;
		resize_map(map, new_size_tiles, offset_tiles);
		_tiles = mutable_tile_map{ map };
	}

	template<unary_operator<void, const resources::tile*> OnClick>
	static void add_tile_buttons(gui &g, const std::span<const resources::tile> tiles, float tile_size_f, OnClick on_click)
	{
		static_assert(std::is_invocable_v<OnClick, resources::tile*>);

		constexpr auto button_size = gui::vector2{ 25.f, 25.f };

		for (const auto& t : tiles)
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
				std::invoke(on_click, &t);
			g.pop_id();
		}
		return;
	}

	void level_editor_tiles::gui_update(gui &g, editor_windows &w)
	{
		using namespace std::string_view_literals;

		if (g.main_toolbar_begin())
		{
			//TODO: a good way to indicate drawing with the tile eraser?
			if (g.toolbar_button("tiles eraser"sv))
			{
				activate_brush();
				_tile = &resources::get_empty_tile();
			}
		}

		g.main_toolbar_end();

		const auto tile_size_f = float_cast(_settings->tile_size);

		if (g.window_begin(editor::gui_names::toolbox))
		{
			if (g.collapsing_header("tiles"sv))
			{
				constexpr auto draw_shapes = std::array{
					"square"sv,
					"circle"sv
				};

				if (g.combo_begin("drawing shape"sv, draw_shapes[static_cast<std::size_t>(_shape)]))
				{
					if (g.selectable(draw_shapes[0], _shape == draw_shape::square))
						_shape = draw_shape::square;

					if (g.selectable(draw_shapes[1], _shape == draw_shape::circle))
						_shape = draw_shape::circle;

					g.combo_end();
				}

				constexpr auto size_min = int{ 0 };
				constexpr auto size_max = int{ 6 };

				g.input_scalar("drawing size"sv, _size);
				_size = std::clamp(_size, size_min, size_max);

				auto on_click = [this](const resources::tile *t) {
					activate_brush();
					_tile = t;
				};

				//each tileset
				//the tiles from a terrain wont appear here unless
				//they are listed under the tiles: tag in the terrain
				for (const auto& tileset : _settings->tilesets)
				{
					const auto& name = data::get_as_string(tileset->id);

					g.indent();
					if (g.collapsing_header(name))
						add_tile_buttons(g, tileset->tiles, tile_size_f, on_click);
				}
			}
		}
		g.window_end();
		
		if (w.new_level && g.window_begin(editor::gui_names::new_level))
		{
			auto dialog_pos = g.window_position().x;
			auto dialog_size = g.get_item_rect_max().x;
			//select default tile
			auto tilesets = _settings->tilesets;
			tilesets.push_back(_settings->empty_tileset);
			
			using namespace std::string_literals;
			auto string = "tileset"s;
			if (_new_options.tileset != nullptr)
				string = data::get_as_string(_new_options.tileset->id);

			if (g.combo_begin("tilesets"sv, string))
			{
				const auto empty_tileset = _settings->empty_tileset;
				if (g.selectable(data::get_as_string(empty_tileset.id()), empty_tileset.get() == _new_options.tileset))
					_new_options.tileset = empty_tileset.get();

				for (const auto tileset : _settings->tilesets)
				{
					if (g.selectable(data::get_as_string(tileset->id), tileset.get() == _new_options.tileset))
						_new_options.tileset = tileset.get();
				}
				
				g.combo_end();
			}

			if (_new_options.tileset == _settings->empty_tileset.get())
				_new_options.tile = &resources::get_empty_tile();
			else if (_new_options.tileset != nullptr)
			{
				auto on_click = [this](const resources::tile *t){
					_new_options.tile = t;
				};

				add_tile_buttons(g, _new_options.tileset->tiles, tile_size_f, on_click);
			}
			
			g.window_end();
		}
	}

	static void draw_on(level_editor_tiles::mouse_pos p, const resources::tile_size_t tile_size, const resources::tile &t,
		level_editor_tiles::draw_shape shape, mutable_tile_map &m, tile_index_t size)
	{
		const auto tile_size_f = float_cast(tile_size);
		const auto pos_int = static_cast<tile_position>(mouse::snap_to_grid(p, tile_size_f) / tile_size_f);

		// TODO: don't use the allocating make_position funcs
		const auto positions = [&] {
			if (level_editor_tiles::draw_shape::square == shape)
				return make_position_square_from_centre(pos_int, size);
			else
				return make_position_circle(pos_int, size);
		} ();

		m.place_tile(positions, t);
	}

	void level_editor_tiles::make_brush_preview(time_duration, mouse_pos p)
	{
		assert(_settings);
		assert(_tile);
		
		_preview = _empty_preview;

		if (p.x < 0
			|| p.y < 0)
			return;

        if (p.x > float_cast(_level_size.x)
            || p.y > float_cast(_level_size.y))
			return;

		draw_on(p, _settings->tile_size, *_tile, _shape, _preview, _size);
	}

	void level_editor_tiles::on_click(mouse_pos p)
	{
		assert(_settings);
		assert(_tile);
		draw_on(p, _settings->tile_size, *_tile, _shape, _tiles, _size);
	}

	void level_editor_tiles::on_drag(mouse_pos p)
	{
		on_click(p);
	}

	void level_editor_tiles::draw(sf::RenderTarget &t, time_duration, sf::RenderStates s)
	{
		t.draw(_tiles, s);
	}

	void level_editor_tiles::draw_brush_preview(sf::RenderTarget &t, time_duration, sf::RenderStates s)
	{
		t.draw(_preview, s);
	}
}
