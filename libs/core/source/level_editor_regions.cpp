#include "hades/level_editor_regions.hpp"

#include "hades/collision.hpp"
#include "hades/exceptions.hpp"
#include "hades/gui.hpp"
#include "hades/level_editor_grid.hpp"
#include "hades/mouse_input.hpp"

namespace hades
{
	level_editor_regions::level_editor_regions()
		: _region_min_size{ console::get_float(cvars::region_min_size) }
	{}

	void level_editor_regions::level_load(const level &l)
	{
		_level_limits = { l.map_x, l.map_y };
	}

	void level_editor_regions::gui_update(gui &g, editor_windows&)
	{
		using namespace std::string_view_literals;

		if (g.main_toolbar_begin())
		{
			if (g.toolbar_button("region tool"sv))
			{
				_brush = brush_type::region_place;
				_show_regions = true;
				activate_brush();
			}

			if (g.toolbar_button("region selector"sv))
			{
				_brush = brush_type::region_selector;
				_show_regions = true;
				activate_brush();
			}

			if (g.toolbar_button("show/hide region"sv))
				_show_regions = !_show_regions;
		}

		g.main_toolbar_end();

		if(g.window_begin(editor::gui_names::toolbox))
		{
			if (g.collapsing_header("regions"sv))
			{
				if (g.listbox("##region_list"sv, _edit.selected, _regions, [](const auto &r)->string {
					return r.name.getString();
				}))
				{
					_on_selected(_edit.selected);
				}
			}

			if (g.collapsing_header("region properties"sv))
			{
				if(_edit.selected == region_edit::nothing_selected)
				{
					g.text("no region selected"sv);
				}
				else
				{
					auto &region = _regions[_edit.selected];
					// change region name
					g.input_text("name"sv, _edit.name);

					const auto end = std::end(_regions);
					auto used_name = std::find_if(std::begin(_regions), end, [name = _edit.name](const auto &r) {
						return name == r.name.getString();
					});

					if (used_name == end)
						region.name.setString(_edit.name);
					else
						g.tooltip("this name is already being used");

					//position
					const auto position = region.shape.getPosition();
					auto pos = std::array{ position.x, position.y };
					if (g.input("position"sv, pos))
					{
						region.shape.setPosition(pos[0], pos[1]);
						_on_selected(_edit.selected);
					}

					//size
					const auto size = region.shape.getSize();
					auto siz = std::array{ size.x, size.y };
					if (g.input("size"sv, siz))
					{
						region.shape.setSize({ siz[0], siz[1] });
						_on_selected(_edit.selected);
					}
				}
			}
		}
		g.window_end();
	}

	enum class line_index : std::size_t {
		top_left,
		top_middle,
		top_right,
		right_top,
		right_middle,
		right_bottom,
		bottom_right,
		bottom_middle,
		bottom_left,
		left_bottom,
		left_middle,
		left_top,
		last
	};

	static void generate_selection_rect(level_editor_regions::region_edit::array_t &rects, rect_float area)
	{
		constexpr auto thickness = 4.f;
		constexpr auto default_length = 8.f;
		
		const auto length = std::min(default_length, std::min(area.width, area.height));

		const auto half_height = area.height / 2.f;
		const auto half_width = area.width / 2.f;
		//top_left
		const auto top_y = area.y - thickness;

		using T = level_editor_regions::region_edit::array_t::size_type;

		sf::RectangleShape l{};
		l.setFillColor(sf::Color::White);
		l.setSize({ length, thickness });
		l.setPosition(area.x, top_y);
		rects[static_cast<T>(line_index::top_left)] = l;

		//top_middle
		l.setPosition(area.x + half_width, top_y);
		rects[static_cast<T>(line_index::top_middle)] = l;

		//top_right
		l.setPosition(area.x + area.width - length, top_y);
		rects[static_cast<T>(line_index::top_right)] = l;

		//bottom_left
		const auto bottom_y = area.y + area.height;
		l.setPosition(area.x, bottom_y);
		rects[static_cast<T>(line_index::bottom_left)] = l;

		//bottom_middle
		l.setPosition(area.x + half_width, bottom_y);
		rects[static_cast<T>(line_index::bottom_middle)] = l;

		//bottom_right
		l.setPosition(area.x + area.width - length, bottom_y);
		rects[static_cast<T>(line_index::bottom_right)] = l;

		//right_top
		const auto right_x = area.x + area.width;
		l.setSize({ thickness, length });
		l.setPosition(right_x, area.y);
		rects[static_cast<T>(line_index::right_top)] = l;

		//right_middle
		l.setPosition(right_x, area.y + half_height);
		rects[static_cast<T>(line_index::right_middle)] = l;

		//right_bottom
		l.setPosition(right_x, area.y + area.height);
		rects[static_cast<T>(line_index::right_bottom)] = l;

		//left_top
		const auto left_x = area.x - thickness;
		l.setPosition(left_x, area.y);
		rects[static_cast<T>(line_index::left_top)] = l;

		//left_middle
		l.setPosition(left_x, area.y + half_height);
		rects[static_cast<T>(line_index::left_middle)] = l;

		//left_bottom
		l.setPosition(left_x, area.y + area.height);
		rects[static_cast<T>(line_index::left_bottom)] = l;
	}

	void level_editor_regions::on_click(mouse_pos m)
	{
		if (_show_regions 
			&& _brush == brush_type::region_selector
			&& !_regions.empty())
		{
			const auto begin = std::cbegin(_regions);
			const auto end = std::cend(_regions);
			const auto target = std::find_if(begin, end, [m](auto &&r) {
				const auto pos = r.shape.getPosition();
				const auto size = r.shape.getSize();
				const auto rect = rect_float{ pos.x, pos.y, size.x, size.y };

				return collision_test(rect, m);
			});

			if (target == end)
				return;

			const auto index = std::distance(begin, target);
			_on_selected(index);
		}
	}

	static void update_region(level_editor_regions::region &r, rect_float area)
	{
		r.shape.setPosition(area.x, area.y);
		r.shape.setSize({ area.width, area.height });

		r.name.setPosition(area.x, area.y);
	}

	static vector_float calculate_pos(level_editor_regions::mouse_pos m,
		const grid_vars &g)
	{
		const auto cell_size = calculate_grid_size(g);
		const auto grid_enabled = g.enabled->load();
		const auto grid_snap = g.snap->load();

		return [grid_enabled, grid_snap, m, cell_size] {
			if (grid_enabled && grid_snap)
				return mouse::snap_to_grid(m, cell_size);
			else
				return m;
		}();
	}

	void level_editor_regions::on_drag_start(mouse_pos m)
	{
		const auto snap_pos = calculate_pos(m, _grid);

		const auto pos = vector_float{
			std::clamp(snap_pos.x, 0.f, static_cast<float>(_level_limits.x)),
			std::clamp(snap_pos.y, 0.f, static_cast<float>(_level_limits.y))
		};

		if (_show_regions
			&& _brush == brush_type::region_place
			&& snap_pos == pos)
		{
			//TODO: this block looks quite messy
			const auto region_colour = sf::Color::Blue - sf::Color{ 0, 0, 0, 255 / 2 };

			_edit.selected = _regions.size();
			auto &r = _regions.emplace_back();
			r.shape.setPosition({ pos.x,pos.y });
			const auto size = _region_min_size->load();
			r.shape.setSize({ size, size });
			r.shape.setFillColor(region_colour);
			_edit.held_corner = rect_corners::bottom_right;
			_edit.corner = true;
			_brush = brush_type::region_corner_drag;
			using namespace std::string_literals;
			r.name.setString("Region"s + to_string(++_new_region_name_counter));
		}
		else if (_show_regions
			&& _brush == brush_type::region_selector)
		{
			using index_t = region_edit::array_t::size_type;
			for (auto i = index_t{ 0 }; i < _edit.selection_lines.size(); ++i)
			{
				const auto global_bounds = _edit.selection_lines[i].getGlobalBounds();

				const auto bounds = rect_float{ 
					global_bounds.left, 
					global_bounds.top, 
					global_bounds.width, 
					global_bounds.height 
				};

				if (collision_test(bounds, m))
				{
					_on_drag_begin(i);
					_drag_start = pos;
					break;
				}
			}
		}
	}

	constexpr rect_corners flip_horizontal(const rect_corners c)
	{
		constexpr auto horizontal_swap = std::array{
			rect_corners::top_right,
			rect_corners::top_left,
			rect_corners::bottom_left,
			rect_corners::bottom_right
		};

		assert(c < rect_corners::last);

		return horizontal_swap[c];
	}

	constexpr rect_corners flip_vertical(const rect_corners c)
	{
		constexpr auto vertical_swap = std::array{
			rect_corners::bottom_left,
			rect_corners::bottom_right,
			rect_corners::top_right,
			rect_corners::top_left
		};

		assert(c < rect_corners::last);

		return vertical_swap[c];
	}

	constexpr direction flip(const direction d)
	{
		constexpr auto swap = std::array{
			direction::right,
			direction::left,
			direction::bottom,
			direction::top
		};

		assert(d < direction::last);

		return swap[static_cast<decltype(swap)::size_type>(d)];
	}

	void level_editor_regions::on_drag(mouse_pos m)
	{
		if (_brush == brush_type::region_corner_drag
			|| _brush == brush_type::region_edge_drag)
		{
			const auto snap_pos = calculate_pos(m, _grid);

			const auto pos = vector_float{
				std::clamp(snap_pos.x, 0.f, static_cast<float>(_level_limits.x)),
				std::clamp(snap_pos.y, 0.f, static_cast<float>(_level_limits.y))
			};

			const auto c = _edit.corner;
			const auto edge = _edit.held_edge;
			const auto corner = _edit.held_corner;

			const auto top = [c, edge, corner] {
				if (!c)
					return edge == direction::top;
				else
					return corner == rect_corners::top_left
						|| corner == rect_corners::top_right;
			}();

			const auto bottom = [c, edge, corner] {
				if (!c)
					return edge == direction::bottom;
				else
					return corner == rect_corners::bottom_left
					|| corner == rect_corners::bottom_right;
			}();

			const auto left = [c, edge, corner] {
				if (!c)
					return edge == direction::left;
				else
					return corner == rect_corners::top_left
						|| corner == rect_corners::bottom_left;
			}();

			const auto right = [c, edge, corner] {
				if (!c)
					return edge == direction::right;
				else
					return corner == rect_corners::top_right
					|| corner == rect_corners::bottom_right;
			}();

			auto &r = _regions[_edit.selected].shape;
			auto[x, y, width, height] = r.getGlobalBounds();

			auto x2 = x + width,
				y2 = y + height;

			if (top)
				y = pos.y;

			if (bottom)
				y2 = pos.y;

			if (left)
				x = pos.x;

			if (right)
				x2 = pos.x;

			//swap the selected edge, or corner to allow inverting the region
			if (x > x2)
			{
				std::swap(x, x2);

				if (c)
					_edit.held_corner = flip_horizontal(corner);
				else
					_edit.held_edge = flip(edge);
			}

			if (y > y2)
			{
				std::swap(y, y2);

				if (c)
					_edit.held_corner = flip_vertical(corner);
				else
					_edit.held_edge = flip(edge);
			}

			assert(x2 >= x);
			assert(y2 >= y);

			width = x2 - x;
			height = y2 - y;

			r.setPosition(x, y);
			r.setSize({ width, height });
		}
	}

	void level_editor_regions::on_drag_end(mouse_pos)
	{
		if (_brush == brush_type::region_edge_drag
			|| _brush == brush_type::region_corner_drag)
		{
			//expand shape to meet minimum size setting
			auto &r = _regions[_edit.selected];
			auto [w, h] = r.shape.getSize();

			w = std::max(w, _region_min_size->load());
			h = std::max(h, _region_min_size->load());

			r.shape.setSize({ w, h });

			_brush = brush_type::region_selector;
		}
	}

	void level_editor_regions::draw(sf::RenderTarget &t, time_duration, sf::RenderStates s)
	{
		if (_show_regions)
		{
			for (const auto &region : _regions)
			{
				t.draw(region.shape, s);
				t.draw(region.name, s);
			}

			for (const auto &r : _edit.selection_lines)
				t.draw(r);
		}
	}

	void level_editor_regions::draw_brush_preview(sf::RenderTarget &t, time_duration, sf::RenderStates s)
	{
		if (_edit.selected != region_edit::nothing_selected) 
		{
			for (const auto &rect : _edit.selection_lines)
				t.draw(rect, s);
		}
	}

	void level_editor_regions::_on_selected(index_t i)
	{
		assert(_regions.size() > i);
		_edit.selected = i;
		const auto &reg = _regions[_edit.selected];

		const auto position = reg.shape.getPosition();
		const auto size = reg.shape.getSize();

		generate_selection_rect(_edit.selection_lines, { position.x, position.y, size.x, size.y });

		_edit.name = reg.name.getString();
	}

	void level_editor_regions::_on_drag_begin(region_edit::array_t::size_type index)
	{
		const auto i = static_cast<line_index>(index);

		//top left corner
		if (i == line_index::top_left
			|| i == line_index::left_top)
		{
			_brush = brush_type::region_corner_drag;
			_edit.held_corner = rect_corners::top_left;
		}
		//top right
		else if (i == line_index::top_right
			|| i == line_index::right_top)
		{
			_brush = brush_type::region_corner_drag;
			_edit.held_corner = rect_corners::top_right;
		}
		//bottom right
		else if (i == line_index::bottom_right
			|| i == line_index::right_bottom)
		{
			_brush = brush_type::region_corner_drag;
			_edit.held_corner = rect_corners::bottom_right;
		}
		//bottom left
		else if (i == line_index::bottom_left
			|| i == line_index::left_bottom)
		{
			_brush = brush_type::region_corner_drag;
			_edit.held_corner = rect_corners::bottom_left;
		}
		//top
		else if (i == line_index::top_middle)
		{
			_brush = brush_type::region_edge_drag;
			_edit.held_edge = direction::top;
		}
		//right
		else if (i == line_index::right_middle)
		{
			_brush = brush_type::region_edge_drag;
			_edit.held_edge = direction::right;
		}
		//bottom
		else if (i == line_index::bottom_middle)
		{
			_brush = brush_type::region_edge_drag;
			_edit.held_edge = direction::bottom;
		}
		//left
		else if (i == line_index::left_middle)
		{
			_brush = brush_type::region_edge_drag;
			_edit.held_edge = direction::left;
		}
		else
			throw std::out_of_range{ "line index for region drag out of range" };
	}

	void create_level_editor_regions_variables()
	{
		console::create_property(cvars::region_min_size, cvars::default_value::region_min_size);
	}
}