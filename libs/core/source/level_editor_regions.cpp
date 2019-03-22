#include "hades/level_editor_regions.hpp"

#include "hades/collision.hpp"
#include "hades/data.hpp"
#include "hades/exceptions.hpp"
#include "hades/font.hpp"
#include "hades/gui.hpp"
#include "hades/level_editor_grid.hpp"
#include "hades/mouse_input.hpp"

namespace hades
{
	constexpr auto char_size = 8u;

	level_editor_regions::level_editor_regions()
		: _font{ data::get<resources::font>(resources::default_font_id())},
		_region_min_size{ console::get_float(cvars::region_min_size) }
	{
	
	}

	void level_editor_regions::level_load(const level &l)
	{
		_edit.selected = region_edit::nothing_selected;
		_regions.clear();

		_level_limits = { l.map_x, l.map_y };

		_regions.reserve(l.regions.size());

		for (const auto &r : l.regions)
		{
			auto shape = sf::RectangleShape{};
			shape.setPosition(r.bounds.x, r.bounds.y);
			shape.setSize({ r.bounds.width, r.bounds.height });
			shape.setFillColor({ r.colour.r, r.colour.g, r.colour.b, r.colour.a });

			auto text = sf::Text{};
			text.setString(r.name);
			text.setCharacterSize(char_size);
			text.setPosition(r.bounds.x, r.bounds.y);
			text.setFont(_font->value);

			_regions.emplace_back(region{ shape, text });
		}
	}

	level level_editor_regions::level_save(level l) const
	{
		assert(l.regions.empty());
		l.regions.reserve(_regions.size());
		for (const auto &r : _regions)
		{
			const auto sf_bounds = r.shape.getGlobalBounds();
			const auto bounds = rect_float{ 
				sf_bounds.left,
				sf_bounds.top,
				sf_bounds.width,
				sf_bounds.height
			};

			const auto col = r.shape.getFillColor();
			const auto c = colour{ col.r, col.g, col.b, col.a };
			const string name = r.name.getString().toAnsiString();
			l.regions.emplace_back(level::region{ c, bounds, name });
		}

		return l;
	}

	void level_editor_regions::gui_update(gui &g, editor_windows&)
	{
		using namespace std::string_view_literals;
		using namespace std::string_literals;

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
				_edit.selected = region_edit::nothing_selected;
				activate_brush();
			}

			if (g.toolbar_button("show/hide regions"sv))
				_show_regions = !_show_regions;

			if (g.toolbar_button("remove region")
				&& _edit.selected != region_edit::nothing_selected)
			{
				assert(_edit.selected < _regions.size());

				const auto target = std::begin(_regions) + _edit.selected;
				_regions.erase(target);
				_edit.selected = region_edit::nothing_selected;
			}
		}

		g.main_toolbar_end();

		if(g.window_begin(editor::gui_names::toolbox))
		{
			if (g.collapsing_header("regions"sv))
			{
				constexpr auto listbox_str = "##region_list"sv;
				constexpr auto region_to_string = [](const auto &r)->string {
					return r.name.getString();
				};

				//set index to 0 if nothing is selected
				//this stops an out of bounds error in listbox
				auto index = _edit.selected == region_edit::nothing_selected
					? std::size_t{ 0 } : _edit.selected;

				if (g.listbox(listbox_str, index, _regions, region_to_string))
					_on_selected(index);
			}

			if (g.collapsing_header("region properties"sv))
			{
				if(_edit.selected == region_edit::nothing_selected)
				{
					g.text("no region selected"sv);
				}
				else
				{
					g.push_id(to_string(_edit.selected));
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

					//colour
					const auto colour = region.shape.getFillColor();
					auto col = std::array{ colour.r, colour.g, colour.b, colour.a };
					static_assert(std::is_same_v<uint8, decltype(col)::value_type>);
					if (g.colour_picker4("colour"sv, col))
						region.shape.setFillColor({ col[0], col[1], col[2], col[3] });

					g.pop_id();
				}
			}
		}
		g.window_end();
	}

	//NOTE: this layout must be kept in sync with
	// edge_map and corner_map
	enum class line_index : std::size_t {
		top_middle,
		edge_begin = top_middle,
		right_middle,
		bottom_middle,
		left_middle,
		top_left,
		edge_end = top_left,
		corner_begin = edge_end,
		top_right,
		right_top,
		right_bottom,
		bottom_right,
		bottom_left,
		left_bottom,
		left_top,
		corner_end,
		last = corner_end
	};

	static void generate_selection_rect(level_editor_regions::region_edit::array_t &rects, rect_float area) noexcept
	{
		constexpr auto thickness = 4.f;
		constexpr auto default_length = 8.f;
		
		const auto length = std::min(default_length, std::min(area.width, area.height));

		const auto half_height = area.height / 2.f;
		const auto half_width = area.width / 2.f;
		//top_left
		const auto top_y = area.y - thickness;

		using T = std::decay_t<decltype(rects)>::size_type;

		sf::RectangleShape l{};
		l.setFillColor(sf::Color::White);
		l.setSize({ length, thickness });

		const auto size_offset = length / 2.f;
		
		//top_middle
		l.setPosition(area.x + half_width - size_offset, top_y);
		rects[static_cast<T>(line_index::top_middle)] = l;

		//bottom_middle
		const auto bottom_y = area.y + area.height;
		l.setPosition(area.x + half_width - size_offset, bottom_y);
		rects[static_cast<T>(line_index::bottom_middle)] = l;

		//top_left
		const auto corner_hori_length = length + thickness;
		l.setSize({ corner_hori_length, thickness });
		l.setPosition(area.x - thickness, top_y);
		rects[static_cast<T>(line_index::top_left)] = l;

		//bottom_left
		l.setPosition(area.x - thickness, bottom_y);
		rects[static_cast<T>(line_index::bottom_left)] = l;

		//top_right
		l.setPosition(area.x + area.width - length, top_y);
		rects[static_cast<T>(line_index::top_right)] = l;

		//bottom_right
		l.setPosition(area.x + area.width - length, bottom_y);
		rects[static_cast<T>(line_index::bottom_right)] = l;

		//right_top
		const auto right_x = area.x + area.width;
		l.setSize({ thickness, length });
		l.setPosition(right_x, area.y);
		rects[static_cast<T>(line_index::right_top)] = l;

		//right_middle
		l.setPosition(right_x, area.y + half_height - size_offset);
		rects[static_cast<T>(line_index::right_middle)] = l;

		//left_middle
		const auto left_x = area.x - thickness;
		l.setPosition(left_x, area.y + half_height - size_offset);
		rects[static_cast<T>(line_index::left_middle)] = l;

		//right_bottom
		l.setPosition(right_x, area.y + area.height - length);
		rects[static_cast<T>(line_index::right_bottom)] = l;

		//left_top
		l.setPosition(left_x, area.y);
		rects[static_cast<T>(line_index::left_top)] = l;

		//left_bottom
		l.setPosition(left_x, area.y + area.height - length);
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

	void level_editor_regions::on_drag_start(mouse_pos m)
	{
		if (!_show_regions)
			return;

		const auto snap_pos = snap_to_grid(m, _grid);

		const auto pos = vector_float{
			std::clamp(snap_pos.x, 0.f, static_cast<float>(_level_limits.x)),
			std::clamp(snap_pos.y, 0.f, static_cast<float>(_level_limits.y))
		};

		if (_brush == brush_type::region_place
			&& snap_pos == pos)
		{
			const auto r_size = _regions.size();
			auto &r = _regions.emplace_back();

			r.shape.setPosition(pos.x,pos.y);
			const auto size = _region_min_size->load();
			r.shape.setSize({ size, size });

			const auto region_colour = sf::Color::Blue - sf::Color{ 0, 0, 0, 255 / 2 };
			r.shape.setFillColor(region_colour);
			
			using namespace std::string_literals;
			r.name.setFont(_font->value);
			r.name.setCharacterSize(char_size);
			r.name.setString("Region"s + to_string(++_new_region_name_counter));
			r.name.setPosition(pos.x, pos.y);

			_on_selected(r_size);

			_edit.held_corner = rect_corners::bottom_right;
			//_edit.corner = true;
			_brush = brush_type::region_corner_drag;
		}
		else if (_brush == brush_type::region_selector)
		{
			//body drag
			const auto target = [m, &regions = _regions] {
				const auto begin = std::begin(regions);
				const auto size = std::size(regions);
				for (auto i = std::size_t{ 0 }; i < size; ++i)
				{
					const auto bounds = regions[i].shape.getGlobalBounds();
					const auto rect = rect_float{
						bounds.left,
						bounds.top,
						bounds.width,
						bounds.height
					};

					if (collision_test(rect, m))
						return i;
				}

				return region_edit::nothing_selected;
			}();

			if (target != region_edit::nothing_selected)
			{
				//start region drag
				const auto region_pos = _regions[target].shape.getPosition();

				//offset between the mouse
				//and the origion of the region{0, 0}
				_edit.drag_offset = { m.x - region_pos.x, m.y - region_pos.y };

				_edit.selected = target;
				_brush = brush_type::region_move;
				_on_selected(target);
			}
			//edge stretch
			else if (_edit.selected != region_edit::nothing_selected)
			{
				//we can only corner drag the selected rect
				//check for edge and corner grabs
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
						break;
					}
				}
			}
		}//if(region_selector)
	}//on_drag_start()

	constexpr rect_corners flip_horizontal(const rect_corners c) noexcept
	{
		//NOTE: this will break if the layout of rect_corners changes
		constexpr auto horizontal_swap = std::array{
			rect_corners::top_right,
			rect_corners::top_left,
			rect_corners::bottom_left,
			rect_corners::bottom_right
		};

		assert(c < rect_corners::last);

		return horizontal_swap[static_cast<decltype(horizontal_swap)::size_type>(c)];
	}

	constexpr rect_corners flip_vertical(const rect_corners c) noexcept
	{
		//NOTE: this will break if the layout of rect_corners changes
		constexpr auto vertical_swap = std::array{
			rect_corners::bottom_left,
			rect_corners::bottom_right,
			rect_corners::top_right,
			rect_corners::top_left
		};

		assert(c < rect_corners::last);

		return vertical_swap[static_cast<decltype(vertical_swap)::size_type>(c)];
	}

	constexpr direction flip(const direction d) noexcept
	{
		//NOTE: this will break if the layout of direction changes
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
		const auto snap_pos = snap_to_grid(m, _grid);

		const auto pos = vector_float{
			std::clamp(snap_pos.x, 0.f, static_cast<float>(_level_limits.x)),
			std::clamp(snap_pos.y, 0.f, static_cast<float>(_level_limits.y))
		};

		if (_brush == brush_type::region_move)
		{
			const auto position = pos - _edit.drag_offset;
			const auto size = _regions[_edit.selected].shape.getSize();

			const auto target_bounds = rect_float{ 
				position.x, position.y,
				size.x, size.y 
			};

			const auto clamp_area = rect_float{ 
				0.f, 0.f,
				static_cast<float>(_level_limits.x),
				static_cast<float>(_level_limits.y) 
			};

			const auto final_bounds = clamp_rect(target_bounds, clamp_area);
			_regions[_edit.selected].shape.setPosition(final_bounds.x, final_bounds.y);
			_regions[_edit.selected].name.setPosition(final_bounds.x, final_bounds.y);

			_on_selected(_edit.selected);
		}
		else if (_brush == brush_type::region_corner_drag
			|| _brush == brush_type::region_edge_drag)
		{
			const auto c = _brush == brush_type::region_corner_drag;
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
			_regions[_edit.selected].name.setPosition(x, y);

			_on_selected(_edit.selected);
		}
	}

	void level_editor_regions::on_drag_end(mouse_pos)
	{
		if (_brush == brush_type::region_move)
			_brush = brush_type::region_selector;
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
		activate_brush();

		_edit.selected = i;

		const auto &reg = _regions[_edit.selected];

		const auto position = reg.shape.getPosition();
		const auto size = reg.shape.getSize();

		generate_selection_rect(_edit.selection_lines, { position.x, position.y, size.x, size.y });

		_edit.name = reg.name.getString();
	}

	void level_editor_regions::_on_drag_begin(region_edit::array_t::size_type index)
	{
		//NOTE: both of these tables will not work,
		// if the layout of line_index changes
		constexpr auto edge_map = std::array{
			direction::top,
			direction::right,
			direction::bottom,
			direction::left,
		};

		constexpr auto corner_offset = edge_map.size();

		constexpr auto corner_map = std::array{
			rect_corners::top_left,
			rect_corners::top_right,
			rect_corners::top_right,
			rect_corners::bottom_right,
			rect_corners::bottom_right,
			rect_corners::bottom_left,
			rect_corners::bottom_left,
			rect_corners::top_left,
		};

		const auto i = static_cast<line_index>(index);

		if (i >= line_index::edge_begin
			&& i < line_index::edge_end)
		{
			_brush = brush_type::region_edge_drag;
			_edit.held_edge = edge_map[index];
		}
		else if (i >= line_index::corner_begin
			&& i < line_index::corner_end)
		{
			_brush = brush_type::region_corner_drag;
			assert(index >= corner_offset);
			_edit.held_corner = corner_map[index - corner_offset];
		}
		else
			throw invalid_argument{ "line index for region drag out of range" };	
	}

	void create_level_editor_regions_variables()
	{
		console::create_property(cvars::region_min_size, cvars::default_value::region_min_size);
	}
}