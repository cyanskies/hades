#include "hades/level_editor_level_properties.hpp"

#include "hades/animation.hpp"
#include "hades/gui.hpp"
#include "hades/level_scripts.hpp"

namespace hades
{
	static void make_level_detail_window(gui &g, bool &open, string &name, string &description, unique_id &player_input, unique_id &ai_input)
	{
		static const auto& player_input_list = resources::get_player_input_list();
		//static const auto &ai_input = resources::get_ai_input_list();

		if (!open)
			return;

		using namespace std::string_view_literals;
		if (g.window_begin("level details"sv, open))
		{
			if (g.button("Done"sv))
				open = false;

			g.input_text("Name"sv, name, gui::input_text_flags::auto_select_all);
			g.input_text_multiline("Description"sv, description, vector_float{}, gui::input_text_flags::no_horizontal_scroll);

			const auto preview = [player_input]()->string {
				if (player_input == unique_id::zero)
					return "none";
				else
					return data::get_as_string(player_input);
			}();

			if (g.combo_begin("player input", preview))
			{
				if (g.selectable("none"sv, unique_id::zero == player_input))
					player_input = unique_id::zero;

				for (const auto& id : player_input_list)
				{
					if (g.selectable(data::get_as_string(id), id == player_input))
						player_input = id;
				}

				g.combo_end();
			}

			if (g.combo_begin("ai input", "none"))
			{
				g.selectable("none"sv, true);
				g.combo_end();
			}
		}
		g.window_end();
	}

	static void apply(const level_editor_level_props::background_settings &s,
		background &b)
	{
		b.set_colour(s.col);
		b.clear();

		for (const auto &l : s.layers)
		{
			try
			{
				b.add({ l.offset, l.parallax, l.anim });
			}
			catch (const data::resource_null &e)
			{
				LOGERROR(e.what());
			}
			catch (const data::resource_wrong_type &e)
			{
				LOGERROR(e.what());
			}
		}
	}

	static void make_layer_edit_pane(gui &g,
		level_editor_level_props::background_settings &uncommitted,
		level_editor_level_props::background_settings_window &window)
	{
		using namespace std::string_view_literals;

		auto &current_layer = uncommitted.layers[window.selected_layer];

		//edit controls for a layer
		g.input_text("animation"sv, window.animation_input);

		const auto id = data::get_uid(window.animation_input);
		if (id == unique_id::zero)
			g.tooltip("this is not a registered id"sv);
		else
		{
			const auto[anim, error] = data::try_get<resources::animation>(id);
			if (!anim)
			{
				using ec = data::data_manager::get_error;
				if (error == ec::no_resource_for_id)
					g.tooltip("this id isn't holding a resource"sv);
				else if (error == ec::resource_wrong_type)
					g.tooltip("this id isn't for a animation"sv);
				else
					LOGERROR("Unexpected value for data::data_manager::get_error, was: " + to_string(static_cast<std::underlying_type_t<ec>>(error)));
			}
			else if(current_layer.anim != id)
				//assign this animation to the current layer
				current_layer.anim = id;
		}

		g.input("x offset"sv, current_layer.offset.x);
		g.input("y offset"sv, current_layer.offset.y);
		g.input("x parallax"sv, current_layer.parallax.x);
		g.input("y parallax"sv, current_layer.parallax.y);
	}

	static void make_background_detail_window(gui &g,
		level_editor_level_props::background_settings &settings,
		level_editor_level_props::background_settings &uncommitted,
		level_editor_level_props::background_settings_window &window,
		background &background)
	{
		if (!window.open)
			return;

		using namespace std::string_view_literals;
		if (g.window_begin("background"sv, window.open))
		{
			if (g.button("Done"sv))
				window.open = false;

			g.layout_horizontal();

			if (g.button("Apply"sv))
			{
				apply(uncommitted, background);
				settings = uncommitted;
			}

			auto &col = uncommitted.col;
			//backdrop colour picker
			auto colour = std::array{ col.r, col.g, col.b };

			g.text("background_colour"sv);
			constexpr auto picker_flags = gui::colour_edit_flags::no_label;
			if (g.colour_picker3("backdrop colour"sv, colour, picker_flags))
			{
				col.r = colour[0];
				col.g = colour[1];
				col.b = colour[2];
			}

			auto &selected = window.selected_layer;
			auto &layers = uncommitted.layers;

			assert(selected <= size(layers));
			
			//layer editor
			if (g.button("move up"sv))
			{
				const auto beg = std::begin(layers);
				const auto at = beg + selected;
				if (at != beg)
				{
					std::iter_swap(at, at - 1);
					--selected;
				}
			}

			g.layout_horizontal();

			if (g.button("add"sv))
			{
				if (std::empty(layers))
					layers.emplace_back();
				else
				{
					const auto at = std::begin(layers) + selected++;
					layers.emplace(at);
				}
			}

			if (g.button("move down"sv))
			{
				const auto rbeg = std::rbegin(layers);
				const auto at = rbeg + (std::size(layers) - selected - 1);
				if (at != rbeg)
				{
					std::iter_swap(at, at - 1);
					++selected;
				}
			}

			g.layout_horizontal();

			if (g.button("remove"sv) && !std::empty(layers))
			{
				const auto at = std::begin(layers) + selected;
				layers.erase(at);

				if (selected == std::size(layers)
					&& selected > 0)
					--selected;
			}

			g.columns_begin(2, false);

			if (g.listbox("layers"sv, selected, layers, [](const auto &l) {
				return to_string(l.anim);
			}))
			{
				//selected item changed
				window.animation_input = to_string(uncommitted.layers[selected].anim);
			}

			g.columns_next();

			if (!layers.empty())
				make_layer_edit_pane(g, uncommitted, window);

			g.columns_end();
		}
		g.window_end();
	}

	level level_editor_level_props::level_new(level l) const
	{
		l.name = _new_name;
		l.description = _new_desc;
		return l;
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

		_player_input = l.player_input_script;
		_ai_input = l.ai_input_script;
	}

	level level_editor_level_props::level_save(level l) const
	{
		l.name = _level_name;
		l.description = _level_desc;
		l.player_input_script = _player_input;
		l.ai_input_script = _ai_input;
		return l;
	}

	void level_editor_level_props::level_resize(vector_int s, vector_int)
	{
		_background.set_size({ static_cast<float>(s.x), static_cast<float>(s.y) });
		return;
	}

	static bool make_background_pick_window(gui &g, level_editor_level_props::background_pick_window &w)
	{
		auto ret = false;

		if (!w.open)
			return ret;

		using namespace std::string_view_literals;

		if (g.window_begin("background"sv, w.open))
		{
			if (g.button("cancel"sv))
				w.open = false;

			if (g.button("apply"sv))
				ret = true;

			g.input_text("##resource"sv, w.input);

			const auto id = data::get_uid(w.input);
			if (id == unique_id::zero)
				g.tooltip("this is not a registered id"sv);
			else
			{
				const auto[back, error] = data::try_get<resources::background>(id);
				if (!back)
				{
					using ec = data::data_manager::get_error;
					if (error == ec::no_resource_for_id)
						g.tooltip("this id isn't holding a resource"sv);
					else if (error == ec::resource_wrong_type)
						g.tooltip("this id isn't for a background"sv);
					else
						LOGERROR("Unexpected value for data::data_manager::get_error, was: " + to_string(static_cast<std::underlying_type_t<ec>>(error)));
				}
				else if (w.resource != id)
					w.resource = id;
			}
		}

		g.window_end();

		return ret;
	}

	void level_editor_level_props::gui_update(gui &g, editor_windows &flags)
	{
		using namespace std::string_view_literals;
		g.main_menubar_begin();
		if (g.menu_begin("level"sv))
		{
			if (g.menu_item("details..."sv))
				_details_window = true;
				
			if (g.menu_item("pick background..."))
				_background_pick_window.open = true;

			if (g.menu_item("custom background..."sv))
			{
				_background_window.open = true;
				_background_uncommitted = _background_settings;
			}

			g.menu_end();
		}

		g.main_menubar_end();

		make_level_detail_window(g, _details_window, _level_name, _level_desc, _player_input, _ai_input);
		make_background_detail_window(g, _background_settings,
			_background_uncommitted, _background_window, _background);

		if (make_background_pick_window(g, _background_pick_window) 
			&& _background_pick_window.resource != unique_id::zero)
		{
			const auto *back = data::get<resources::background>(_background_pick_window.resource);
			assert(back);

			background_settings b;
			b.col = back->colour;
			b.layers.reserve(back->layers.size());

			for (const auto &l : back->layers)
			{
				assert(l.animation);

				const auto layer = background_settings::background_layer{
					l.animation->id,
					l.offset,
					l.parallax
				};

				b.layers.emplace_back(layer);
			}

			std::swap(b, _background_uncommitted);
			apply(_background_uncommitted, _background);
			_background_settings = _background_uncommitted;
		}

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
		const auto view = t.getView();
		const auto size = view.getSize();
		const auto view_pos = view.getCenter() - size / 2.f;

		_background.update(time_point{}, { view_pos.x, view_pos.y, size.x, size.y });
		t.draw(_background, s);
	}
}
