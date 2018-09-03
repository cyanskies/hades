#include "Objects/resources.hpp"

#include "Hades/Data.hpp"
#include "Hades/data_system.hpp"
#include "Hades/Properties.hpp"

#include "Objects/editor.hpp"

namespace objects
{
	namespace resources
	{
		void ParseEditor(hades::unique_id mod, const YAML::Node &node, hades::data::data_manager *data);
		void ParseObject(hades::unique_id mod, const YAML::Node &node, hades::data::data_manager *data);
	}

	void MakeDefaultCurves(hades::data::data_manager* data)
	{
		//Builtin curves exist for the values needed by the editor
		//these curves are auto generated for any object placed in the editor

		using hades::resources::curve;
		using namespace hades::resources::curve_types;

		//position
		const auto position_id = data->get_uid("position");
		auto position_c = hades::data::FindOrCreate<curve>(position_id, hades::empty_id, data);
		position_c->curve_type = hades::curve_type::linear;
		position_c->data_type = hades::resources::VariableType::VECTOR_INT;
		position_c->default_value.set = true;
		position_c->default_value.value = std::vector<int_t>{ 0, 0 };
		//size
		const auto size_id = data->get_uid("size");
		auto size_c = hades::data::FindOrCreate<curve>(size_id, hades::empty_id, data);
		size_c->curve_type = hades::curve_type::linear;
		size_c->data_type = hades::resources::VariableType::VECTOR_INT;
		size_c->default_value.set = true;
		size_c->default_value.value = std::vector<int_t>{ 8, 8 };
	}

	void DefineObjectConsoleVars()
	{
		//object editor variables
		//editor ui
		hades::console::set_property(editor_scroll_margin, editor_scroll_margin_default);
		hades::console::set_property(editor_scroll_rate, editor_scroll_rate_default);
		hades::console::set_property(editor_camera_height, editor_camera_height_default);
		hades::console::set_property(editor_zoom_max, editor_zoom_max_default);
		hades::console::set_property(editor_zoom_start, editor_zoom_default);
		//grid
		hades::console::set_property(editor_snaptogrid, editor_snap_default);
		hades::console::set_property(editor_grid_size, editor_grid_default);
		hades::console::set_property(editor_grid_size_multiple, 1);
		hades::console::set_property(editor_grid_max, 3);
		hades::console::set_property(editor_grid_enabled, true);
		hades::console::set_property(editor_grid_size_multiple, 1);
		//map
		hades::console::set_property(editor_map_size, editor_map_size_default);
		//objects
		hades::console::set_property(editor_object_mock_size, editor_mock_size_default);
	}

	void RegisterObjectResources(hades::data::data_system *data)
	{
		DefineObjectConsoleVars();

		data->register_resource_type("editor", resources::ParseEditor);
		data->register_resource_type("objects", resources::ParseObject);

		MakeDefaultCurves(data);
	}

	namespace resources
	{
		std::vector<const object*> Objects;
		std::vector<object_group> ObjectGroups;

		template<class T>
		hades::resources::curve_default_value ParseValue(const YAML::Node &node)
		{
			hades::resources::curve_default_value out;
			try
			{
				out.set = true;
				out.value.emplace<T>(node.as<T>());
			}
			catch (YAML::InvalidNode&)
			{
				return hades::resources::curve_default_value();
			}

			return out;
		}

		template<>
		hades::resources::curve_default_value ParseValue<hades::unique_id>(const YAML::Node &node)
		{
			hades::resources::curve_default_value out;
			try
			{
				const auto s = node.as<hades::types::string>();
				const auto u = hades::data::get_uid(s);
				if (u == hades::empty_id)
					return out;

				out.set = true;
				out.value.emplace<hades::unique_id>(u);
			}
			catch (YAML::InvalidNode&)
			{
				return hades::resources::curve_default_value();
			}

			return out;
		}

		template<class T>
		hades::resources::curve_default_value ParseValueVector(const YAML::Node &node)
		{
			hades::resources::curve_default_value out;
			try
			{
				std::vector<T> vec;
				for (auto v : node)
					vec.push_back(node.as<T>());

				out.set = true;
				out.value.emplace<std::vector<T>>(vec);
			}
			catch (YAML::InvalidNode&)
			{
				return hades::resources::curve_default_value();
			}

			return out;
		}

		template<>
		hades::resources::curve_default_value ParseValueVector<hades::unique_id>(const YAML::Node &node)
		{
			hades::resources::curve_default_value out;
			try
			{
				std::vector<hades::unique_id> vec;
				for (auto v : node)
				{
					const auto s = node.as<hades::types::string>();
					const auto u = hades::data::get_uid(s);
					if (u != hades::empty_id)
						vec.push_back(u);
				}

				out.set = true;
				out.value.emplace<std::vector<hades::unique_id>>(vec);
			}
			catch (YAML::InvalidNode&)
			{
				return hades::resources::curve_default_value();
			}

			return out;
		}

		hades::resources::curve_default_value ParseDefaultValue(const YAML::Node &node, const hades::resources::curve* c)
		{
			if (c == nullptr)
				return hades::resources::curve_default_value();

			using hades::resources::VariableType;

			switch (c->data_type)
			{
			case VariableType::BOOL:
				return ParseValue<bool>(node);
			case VariableType::INT:
				[[fallthrough]];
			case VariableType::OBJECT_REF:
				return ParseValue<hades::types::int32>(node);
			case VariableType::FLOAT:
				return ParseValue<float>(node);
			case VariableType::STRING:
				return ParseValue<hades::types::string>(node);
			case VariableType::UNIQUE:
				return ParseValue<hades::unique_id>(node);
			case VariableType::VECTOR_INT:
				[[fallthrough]];
			case VariableType::VECTOR_OBJECT_REF:
				return ParseValueVector<hades::types::int32>(node);
			case VariableType::VECTOR_FLOAT:
				return ParseValueVector<float>(node);
			case VariableType::VECTOR_UNIQUE:
				return ParseValueVector<hades::unique_id>(node);
			}

			return hades::resources::curve_default_value();
		}

		object::curve_obj ParseCurveObj(const YAML::Node &node, hades::data::data_manager *data)
		{
			object::curve_obj c;
			std::get<0>(c) = nullptr;

			auto getPtr = [data](std::string_view s)->hades::resources::curve* {
				if (s.empty())
					return nullptr;
				
				const auto id = data->get_uid(s);
				return data->get<hades::resources::curve>(id);
			};

			if (node.IsScalar())
			{
				const auto id_str = node.as<hades::types::string>("");
				std::get<0>(c) = getPtr(id_str);
			}
			else if (node.IsSequence())
			{
				const auto first = node[0];
				if (!first.IsDefined())
					return c;

				const auto id_str = first.as<hades::types::string>("");
				std::get<0>(c) = getPtr(id_str);

				const auto second = node[1];
				if (!second.IsDefined())
					return c;

				std::get<1>(c) = ParseDefaultValue(second, std::get<0>(c));
			}

			return c;
		}

		void ParseEditor(hades::unique_id mod, const YAML::Node &node, hades::data::data_manager *data)
		{
			//editor:
			//    scroll-rate: +int
			//    scroll-margin: +int
			//    camera_height: +int
			//    zoom_start: +int
			//    zoom_max: +int
			//
			//    object-groups:
			//        <group-name>: object or [object1, object2, ...]
			//    object-mock-size: 8
			//    object-mock-colour:[r,g,b,a]
			//
			//    snap-to-grid: [-1, 0, 1, 2], [force-disabled, disabled, enabled, force-enabled]
			//    grid-min-size: default 8 or something, defines the grids smallest size
			//    grid-max-size: //how many multiples of grid-min-size can the grid be, eg. 5 = 8*5 is the largest size
			//	  show-grid-settings: true // hides the toolbar buttons for modifying the grid
			//    show-grid: false // defines whether the grid should be turned on by default
			//    grid-colour: [r,g,b,a]
			//    grid-highlight: [r,g,b,a]
			//    //grid toolbar icons
			//	  grid-show-icon: animation name
			//    grid-hide-icon: animation name
			//    grid-grow-icon: animation name
			//    grid-shrink-icon: animation name
			//
			//    //default map settings
			//    map-size:
			
			//use MakeColour to turn vectors into sf::color

			constexpr auto resource_type = "editor";
			constexpr auto resource_name = "N/A";
			auto editor_id = data->get_uid(resource_type);

			if (!yaml_error(resource_type, "n/a", "n/a", "map", mod, node.IsMap()))
				return;

			//Parse editor variables
			const auto scroll_rate = yaml_get_scalar<hades::types::int32>(node, resource_type, resource_name, "scroll-rate", mod, 
				*hades::console::get_int(editor_scroll_rate, editor_scroll_rate_default));
			hades::console::set_property(editor_scroll_rate, scroll_rate);

			const auto scroll_margin = yaml_get_scalar<hades::types::int32>(node, resource_type, resource_name, "scroll-margin", mod,
				*hades::console::get_int(editor_scroll_margin, editor_scroll_margin_default));
			hades::console::set_property(editor_scroll_margin, scroll_margin);

			//Parse Object variables

			const auto groups = node["object-groups"];

			for (auto n : groups)
			{
				const auto namenode = n.first;
				const auto gname = namenode.as<hades::types::string>();
				
				auto g = std::find_if(std::begin(ObjectGroups), std::end(ObjectGroups), [gname](object_group g) {
					return g.name == gname;
				});

				if (g == std::end(ObjectGroups))
				{
					object_group new_g;
					new_g.name = gname;
					ObjectGroups.emplace_back(new_g);
					g = --std::end(ObjectGroups);
				}

				auto v = n.second;

				auto add_to_group = [g, gname, mod, data](const YAML::Node& n) {
					const auto obj_str = n.as<hades::types::string>();
					const auto obj_id = hades::data::make_uid(obj_str);
					const auto obj = hades::data::FindOrCreate<object>(obj_id, mod, data);
					g->obj_list.push_back(obj);
				};

				if (v.IsScalar())
					add_to_group(v);
				else if (v.IsSequence())
					for (const auto o : v)
						add_to_group(o);
			}//for object groups

			const auto mock_size = yaml_get_scalar<hades::types::int32>(node, resource_type, resource_name, "object-mock-size", mod,
				*hades::console::get_int(editor_object_mock_size, editor_mock_size_default));
			hades::console::set_property(editor_object_mock_size, mock_size);

			//Parse Grid Settings

			const auto snap = node["snap-to-grid"];
			if (snap.IsDefined() && snap.IsScalar())
			{
				auto intval = snap.as<hades::types::int32>(-1);

				if (intval < GRIDSNAP_FORCE_DISABLED || intval > GRIDSNAP_FORCE_ENABLED)
				{
					//intval is out of range, check to see if its a valid string
					const auto str = snap.as<hades::types::string>(hades::types::string());
					if (str == "force-disabled")
						intval = GRIDSNAP_FORCE_DISABLED;
					else if (str == "disabled")
						intval = GRIDSNAP_DISABLED;
					else if (str == "enabled")
						intval = GRIDSNAP_ENABLED;
					else if (str == "forced-enabled")
						intval = GRIDSNAP_FORCE_ENABLED;
				}

				if(intval >= GRIDSNAP_FORCE_DISABLED && intval <= GRIDSNAP_FORCE_ENABLED)
					hades::console::set_property(editor_snaptogrid, intval);
			}

			// Get the minimum grid size for the editor(this should be the same value the game uses if it uses
			// grid based game logic
			const hades::types::int32 current_grid_size = *hades::console::get_int(editor_grid_size, editor_grid_default);
			const auto grid_size = yaml_get_scalar<hades::types::int32>(node, resource_type, resource_name, "grid-min-size", mod, current_grid_size);

			if (grid_size != current_grid_size)
			{
				hades::console::set_property(editor_grid_size, grid_size);
				hades::console::set_property(editor_grid_size_multiple, grid_size);
			}

			const bool grid_enabled = yaml_get_scalar<bool>(node, resource_type, resource_name, "show-grid", mod,
				*hades::console::get_bool(editor_grid_enabled, true));
			hades::console::set_property(editor_grid_enabled, grid_enabled);

			//Parse Map Settings
			const auto map_size = yaml_get_scalar<hades::types::int32>(node, resource_type, resource_name, "map-size", mod,
				*hades::console::get_int(editor_map_size, editor_map_size_default));
			hades::console::set_property(editor_map_size, map_size);

			//========
			// editor settings object
			//========

			auto editor_obj = hades::data::FindOrCreate<editor>(editor_id, mod, data);

			//grid stuff
			editor_obj->show_grid_settings = yaml_get_scalar<bool>(node, resource_type, resource_name, "show-grid-settings", mod, editor_obj->show_grid_settings);
			
			const auto grid_colour = yaml_get_sequence<hades::types::int32>(node, resource_type, resource_name, "grid-colour", mod);
			if (!grid_colour.empty())
				editor_obj->grid = hades::MakeColour(std::begin(grid_colour), std::end(grid_colour));

			const auto grid_highlight = yaml_get_sequence<hades::types::int32>(node, resource_type, resource_name, "grid-highlight", mod);
			if (!grid_highlight.empty())
				editor_obj->grid_highlight = hades::MakeColour(std::begin(grid_highlight), std::end(grid_highlight));

			//object settings
			const auto object_mock = yaml_get_sequence<hades::types::int32>(node, resource_type, resource_name, "object-mock-colour", mod);
			if (!object_mock.empty())
				editor_obj->object_mock_colour = hades::MakeColour(std::begin(object_mock), std::end(object_mock));
			
			//======================
			//get grid toolbar icons
			//======================
			using hades::resources::animation;
			const auto grid_shrink_id = yaml_get_uid(node, resource_type, resource_name, "grid-shrink-icon", mod);
			if (grid_shrink_id != hades::unique_id::zero)
				editor_obj->grid_shrink_icon = hades::data::FindOrCreate<animation>(grid_shrink_id, mod, data);

			const auto grid_grow_id = yaml_get_uid(node, resource_type, resource_name, "grid-grow-icon", mod);
			if (grid_grow_id != hades::unique_id::zero)
				editor_obj->grid_grow_icon = hades::data::FindOrCreate<animation>(grid_grow_id, mod, data);

			const auto grid_show_id = yaml_get_uid(node, resource_type, resource_name, "grid-show-icon", mod);
			if (grid_show_id != hades::unique_id::zero)
				editor_obj->grid_show_icon = hades::data::FindOrCreate<animation>(grid_show_id, mod, data);
		}//parse editor

		void ParseObject(hades::unique_id mod, const YAML::Node &node, hades::data::data_manager *data)
		{
			//objects:
			//	thing:
			//		editor-icon : icon_anim
			//		editor-anim : scalar animationId OR sequence [animId1, animId2, ...]
			//		base : object_base OR [obj1, obj2, ...]
			//		curves:
			//			-[curve id, default value]
			//			-curve_id(no default value)
			//		systems: [system_id, ...]
			//		client-systems : [system_id, ...] // aka. render system

			constexpr auto resource_type = "objects";

			if (!yaml_error(resource_type, "n/a", "n/a", "map", mod, node.IsMap()))
				return;

			for (auto n : node)
			{
				const auto namenode = n.first;
				const auto name = namenode.as<hades::types::string>();
				const auto id = data->get_uid(name);

				const auto obj = hades::data::FindOrCreate<object>(id, mod, data);

				if (!obj)
					continue;

				Objects.push_back(obj);

				const auto v = n.second;

				//=========================
				//get the icon used for the editor
				//=========================
				hades::unique_id icon_id = hades::unique_id::zero;

				if (obj->editor_icon)
					icon_id = obj->editor_icon->id;

				icon_id = yaml_get_uid(v, resource_type, name, "editor-icon", mod, icon_id);

				if (icon_id != hades::unique_id::zero)
					obj->editor_icon = data->get<hades::resources::animation>(icon_id);

				//=========================
				//get the animation list used for the editor
				//=========================
				const auto anim_list_ids = yaml_get_sequence<hades::types::string>(v, resource_type, name, "editor-anim", mod);
				
				const auto anims = convert_string_to_resource<hades::resources::animation>(std::begin(anim_list_ids), std::end(anim_list_ids), data);
				std::move(std::begin(anims), std::end(anims), std::back_inserter(obj->editor_anims));

				//remove any duplicates
				hades::remove_duplicates(obj->editor_anims);
				
				//===============================
				//get the base objects
				//===============================

				const auto base_list_ids = yaml_get_sequence<hades::types::string>(v, resource_type, name, "base", mod);
				const auto base = convert_string_to_resource<object>(std::begin(base_list_ids), std::end(base_list_ids), data);
				std::copy(std::begin(base), std::end(base), std::back_inserter(obj->base));

				//remove dupes
				hades::remove_duplicates(obj->base);
				
				//=================
				//get the curves
				//=================

				const auto curvesNode = v["curves"];
				if (yaml_error(resource_type, name, "curves", "map", mod, node.IsMap()))
				{
					for (auto c : curvesNode)
					{
						const auto curve = ParseCurveObj(c, data);

						const auto curve_ptr = std::get<0>(curve);

						//if the curve already exists then replace it's default value
						auto target = std::find_if(std::begin(obj->curves), std::end(obj->curves), [curve_ptr](object::curve_obj c) {
							return curve_ptr == std::get<0>(c);
						});

						if (target == std::end(obj->curves))
							obj->curves.push_back(curve);
						else
							*target = curve;
					}
				}
				
				//TODO:
				//=================
				//get the systems
				//=================

				//=================
				//get the client systems
				//=================
			}
		}
	}
}
