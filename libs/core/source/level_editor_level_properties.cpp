#include "hades/level_editor_level_properties.hpp"

#include "hades/gui.hpp"

namespace hades
{
	static void make_level_detail_window(gui &g, bool &open, string &name, string &description)
	{
		if (!open)
			return;

		using namespace std::string_view_literals;
		if (g.window_begin("level details"sv, open))
		{
			if (g.button("Done"sv))
				open = false;

			g.input_text("Name"sv, name, gui::input_text_flags::auto_select_all);
			g.input_text_multiline("Description"sv, description, vector_float{}, gui::input_text_flags::no_horizontal_scroll);
		}
		g.window_end();
	}

	static void apply(const level_editor_level_props::background_settings &s,
		background &b)
	{
		b.set_colour(s.col);
	}

	static void make_background_detail_window(gui &g, bool &open,
		level_editor_level_props::background_settings &settings,
		level_editor_level_props::background_settings &uncommitted,
		background &background)
	{
		if (!open)
			return;

		using namespace std::string_view_literals;
		if (g.window_begin("background"sv, open))
		{
			if (g.button("Done"sv))
				open = false;

			g.layout_horizontal();

			if (g.button("Apply"sv))
			{
				apply(uncommitted, background);
				settings = uncommitted;
			}

			auto &col = uncommitted.col;
			//backdrop colour picker
			auto colour = std::array{ col.r, col.g, col.b };
			if (g.colour_picker3("backdrop colour"sv, colour))
			{
				col.r = colour[0];
				col.g = colour[1];
				col.b = colour[2];
			}

			//layer editor
		}
		g.window_end();
	}

	void level_editor_level_props::level_load(const level &l)
	{
		_new_name = editor::new_level_name;
		_new_desc = editor::new_level_description;

		_level_name = l.name;
		_level_desc = l.description;

		_background.set_size({ static_cast<float>(l.map_x),
							  static_cast<float>(l.map_y) });

		_background.set_colour(colours::black);
	}

	level level_editor_level_props::level_save(level l) const
	{
		l.name = _level_name;
		l.description = _level_desc;
		return l;
	}

	void level_editor_level_props::gui_update(gui &g, editor_windows &flags)
	{
		using namespace std::string_view_literals;
		g.main_menubar_begin();
		if (g.menu_begin("level"sv))
		{
			if (g.menu_item("details..."sv))
				_details_window = true;
				
			if (g.menu_item("background..."sv))
			{
				_background_window = true;
				_background_uncommitted = _background_settings;
			}

			g.menu_end();
		}

		g.main_menubar_end();

		make_level_detail_window(g, _details_window, _level_name, _level_desc);
		make_background_detail_window(g, _background_window,
			_background_settings, _background_uncommitted, _background);

		if (flags.new_level)
		{
			if (g.window_begin(editor::gui_names::new_level, flags.new_level))
			{
				g.input_text("Name", _new_name, gui::input_text_flags::auto_select_all);
				g.input_text_multiline("Description", _new_desc);
			}
			g.window_end();
		}
	}

	void level_editor_level_props::draw(sf::RenderTarget &t, time_duration, sf::RenderStates s)
	{
		t.draw(_background, s);
	}
}
