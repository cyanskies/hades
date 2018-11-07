#include "hades/level_editor_objects.hpp"

#include "hades/animation.hpp"
#include "hades/gui.hpp"
#include "hades/parser.hpp"

using namespace std::string_view_literals;
constexpr auto level_editor_object_resource_name = "level-editor-object-settings"sv;
static auto object_settings_id = hades::unique_id::zero;

namespace hades::resources
{
	static inline void parse_level_editor_object_resource(unique_id mod, const data::parser_node &node, data::data_manager &d)
	{
		//level-editor-object-settings:
		//    object-groups:
		//        group-name: [elms, elms]

		if (object_settings_id == unique_id::zero)
			object_settings_id = unique_id{};

		auto settings = d.find_or_create<level_editor_object_settings>(object_settings_id, mod);

		const auto object_groups = node.get_child("object-groups"sv);
		if (object_groups)
		{
			const auto o_groups = object_groups->get_children();
			for (const auto &g : o_groups)
			{
				const auto name = g->to_string();
				auto iter = std::find_if(std::begin(settings->groups), std::end(settings->groups), [&name](const auto &t) {
					return std::get<string>(t) == name;
				});

				auto &group = iter != std::end(settings->groups) ? *iter : settings->groups.emplace_back();

				std::vector<const object*> &group_content = std::get<1>(group);
				group_content = g->merge_sequence(std::move(group_content), [](const std::string_view str_id) {
					const auto id = data::get_uid(str_id);
					return data::get<object>(id);
				});
			}
		}//!object_groups
	}
}

namespace hades
{
	void register_level_editor_object_resources(data::data_manager &d)
	{
		d.register_resource_type(level_editor_object_resource_name, resources::parse_level_editor_object_resource);
	}

	level_editor_objects::level_editor_objects()
	{
		if (object_settings_id != unique_id::zero)
			_settings = data::get<resources::level_editor_object_settings>(object_settings_id);

		if (_settings)
			_object_group_selection = std::vector<bool>(_settings->groups.size() + 1, false);
		else
			_object_group_selection = { false };

		for (const auto o : resources::all_objects)
		{
			if (!o->loaded)
				data::get<resources::object>(o->id);
		}
	}

	void add_object_buttons(gui &g, float toolbox_width, const std::vector<const resources::object*> &objects)
	{
		constexpr auto button_size = vector_float{ 25.f, 25.f };
		constexpr auto button_scale_diff = 6.f;
		constexpr auto button_size_no_img = vector_float{ button_size.x + button_scale_diff,
														  button_size.y + button_scale_diff };

		auto x2 = 0.f;
		for (const auto o : objects)
		{
			const auto new_x2 = x2 + button_size.x;
			if (new_x2 < toolbox_width)
				g.layout_horizontal();

			auto clicked = false;

			if (const auto ico = get_editor_icon(*o); ico)
			{
				clicked = g.image_button(*ico, button_size);
				g.tooltip(get_object_name(*o));
			}
			else
			{
				const auto name = get_object_name(*o);
				clicked = g.button(name, button_size_no_img);
				g.tooltip(name);
			}

			if (clicked)
			{
				//TODO:
				//set cursor type
				//set held object
			}

			x2 = g.get_item_rect_max().x;
		}
	}

	void level_editor_objects::gui_update(gui &g)
	{
		using namespace std::string_view_literals;
		//toolbar buttons
		g.main_toolbar_begin();

		if (g.toolbar_button("object selector"sv))
		{
			activate_brush();
			_brush_type = brush_type::object_selector;
		}

		if (g.toolbar_button("region selector"sv))
		{
			activate_brush();
			_brush_type = brush_type::region_selector;
		}

		g.main_toolbar_end();

		std::size_t selected_index = 0u;
		g.window_begin(editor::gui_names::toolbox);

		const auto toolbox_width = g.get_item_rect_max().x;

		if (g.collapsing_header("objects"sv))
		{
			constexpr auto all_str = "all"sv;
			constexpr auto all_index = 0u;

			//if selected == 0 then we're in the special 'all' group
			//otherwise the preview should be set to the name of the selected group
			if (g.combo_begin({}, selected_index == 0u ? all_str : std::get<string>(_settings->groups[selected_index - 1])))
			{
				_object_group_selection[0u] = g.selectable(all_str, _object_group_selection[0u]);

				for (std::size_t i = 1u; i < _object_group_selection.size(); ++i)
				{
					assert(_settings);
					assert(i - 1 < _settings->groups.size());
					_object_group_selection[i] = g.selectable(std::get<string>(_settings->groups[i - 1]), _object_group_selection[i]);
				}

				g.combo_end();
			}

			//add a dummy item, to move the input curser to the next line
			g.dummy();

			for(std::size_t i = 0u; i < _object_group_selection.size(); ++i)
			{
				if (_object_group_selection[i])
					_current_group = i;
			}

			//create selector buttons for objects
			if (_current_group == all_index)
			{
				//'all' is selected
				add_object_buttons(g, toolbox_width, resources::all_objects);
			}
			else
			{
				assert(_settings);
				assert(_current_group == _settings->groups.size() - 1);

				const auto &group = _settings->groups[_current_group - 1];
				add_object_buttons(g, toolbox_width,
					std::get<std::vector<const resources::object*>>(group));
			}
		}

		g.window_end();
	}
}