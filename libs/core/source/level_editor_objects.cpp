#include "hades/level_editor_objects.hpp"

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

		if (g.collapsing_header("objects"sv))
		{
			constexpr auto all_str = "all"sv;

			//if selected == 0 then we're in the special 'all' group
			//otherwise the preview should be set to the same of the selected group
			if (g.combo_begin({}, selected_index == 0u ? all_str : std::get<string>(_settings->groups[selected_index - 1])))
			{
				_object_group_selection[0u] = g.selectable(all_str, _object_group_selection[0u]);

				for (std::size_t i = 1u; i < _object_group_selection.size(); ++i)
				{
					assert(_settings);
					_object_group_selection[i] = g.selectable(std::get<string>(_settings->groups[i - 1]), _object_group_selection[i]);
				}

				g.combo_end();
			}
		}

		g.window_end();
	}
}