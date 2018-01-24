#include "Objects/resources.hpp"

#include "Hades/Data.hpp"
#include "Hades/data_manager.hpp"
#include "Hades/Properties.hpp"

#include "Objects/editor.hpp"

namespace objects
{
	namespace resources
	{
		void ParseEditor(hades::data::UniqueId mod, const YAML::Node &node, hades::data::data_manager *data);
		void ParseObject(hades::data::UniqueId mod, const YAML::Node &node, hades::data::data_manager *data);
	}

	void makeDefaultLayout(hades::data::data_manager* data)
	{
		auto layout_id = data->getUid(editor::object_editor_layout);

		hades::resources::string *layout = hades::data::FindOrCreate<hades::resources::string>(layout_id, hades::data::UniqueId::Zero, data);

		if (!layout)
			return;

        layout->value =
            R"(MenuBar.MenuBar {
                    MinimumSubMenuWidth = 125;
                    Size = (&.size, 20);
                    TextSize = 13;

                    Menu {
                        Items = ["New...", "Load...", "Save", "Save As...", "Exit"];
                        Name = "File";
                    }

                    Menu {
                        Items = ["Reset Gui"];
                        Name = "View";
                    }
                }

                Panel.ToolBar {
                    Position = (0, "MenuBar.y + MenuBar.h");
                    Size = (&.size, 40);
                    SimpleHorizontalLayout.toolbar-container {
                    }
                }

                ChildWindow {
                    Position = (0, "ToolBar.y + ToolBar.h");
                    Size = ("&.w / 6", "&.h - ToolBar.h - MenuBar.h - 20");
                    Visible = true;
                    Resizable = false;
                    Title = "ToolBox";
                    TitleButtons = None;

                    SimpleVerticalLayout.object-selector {
                    }
                }

                ChildWindow."load_dialog" {
                    Position = (200, 200);
                    Title = "Load...";
                    TitleButtons = Minimize;
                    Label."load_mod_label" {
                        Position = (5, 5);
                        Text = "Mod:";
                    }

                    EditBox."load_mod" {
                        Position = (5, "load_mod_label.y + load_mod_label.h + 5");
                        Size = (100, 30);
                        DefaultText = "./";
                    }

                    EditBox."load_filename" {
                        Position = ("load_mod.x + load_mod.w + 10", "load_mod.y");
                        Size = (100, 30);
                        DefaultText = "new.lvl";
                    }

                    Label {
                        Position = ("load_filename.x", 5);
                        Text = "Filename:";
                    }

                    Button."load_button" {
                        Position = ("load_filename.x + load_filename.w + 10" , "load_mod.y");
                        Text = "Load";
                    }

                    Size = ("load_button.x + load_button.w + 5", "load_filename.y + load_filename.h + 5");
                }

                ChildWindow."save_dialog" {
                    Position = (200, 200);
                    Title = "Save...";
                    TitleButtons = Minimize;
                    Label."save_mod_label" {
                        Position = (5, 5);
                        Text = "Mod:";
                    }

                    EditBox."save_mod" {
                        Position = (5, "save_mod_label.y + save_mod_label.h + 5");
                        Size = (100, 30);
                        DefaultText = "./";
                    }

                    EditBox."save_filename" {
                        Position = ("save_mod.x + save_mod.w + 10", "save_mod.y");
                        Size = (100, 30);
                        DefaultText = "new.lvl";
                    }

                    Label {
                        Position = ("save_filename.x", 5);
                        Text = "Filename:";
                    }

                    Button."save_button" {
                        Position = ("save_filename.x + save_filename.w + 10" , "save_mod.y");
                        Text = "Save As";
                    }

                    Size = ("save_button.x + save_button.w + 5", "save_filename.y + save_filename.h + 5");
                }

                ChildWindow."new_dialog" {
                    Position = (150, 150);
                    Title = "New...";
                    TitleButtons = Minimize;

                    Label."new_mod_label" {
                        Text = "Mod:";
                        Position = (5, 5);
                    }

                    EditBox."new_mod" {
                        Position = (5, "new_mod_label.x + new_mod_label.h + 10");
                        Size = (100, 30);
                        DefaultText = "./";
                
                    }

                    EditBox."new_filename" {
                        Position = ("new_mod.x + new_mod.w + 10", "new_mod.y");
                        Size = (100, 30);
                        DefaultText = "new.lvl";
                    }

                    Label {
                        Position = ("new_filename.x", "new_mod_label.y");
                        Text = "Filename:";
                    }

                    Label."new_sizex_label" {
                        Position = ("new_mod_label.x", "new_filename.y + new_filename.h + 10");
                        Text = "Width:";
                    }

                    EditBox."new_sizex" {
                        Position = ("new_sizex_label.x + new_sizex_label.w + 10", "new_sizex_label.y");
                        Size = (50, 30);
                        DefaultText = "200";
                    }

                    Label."new_sizey_label" {
                        Position = ("new_sizex.x + new_sizex.w + 10", "new_sizex_label.y");
                        Text = "Height:";
                    }

                    EditBox."new_sizey" {
                        Position = ("new_sizey_label.x + new_sizey_label.w + 10", "new_sizey_label.y");
                        Size = (50, 30);
                        DefaultText = "200";
                    }

                    Button."new_button" {
                        Position = ("new_filename.x + new_filename.w + 10", "new_filename.y");
                        Text = "Create";
                    }

                    Size = ("new_button.x + new_button.w + 5", "new_sizey.y + new_sizey.h + 5");
                })";
	}

	void DefineObjectConsoleVars()
	{
		//object editor variables
		//grid
		hades::console::SetProperty(editor_snaptogrid, editor_snap_default);
		hades::console::SetProperty(editor_grid_size, editor_grid_default);
		hades::console::SetProperty(editor_grid_enabled, true);
		hades::console::SetProperty(editor_grid_size_multiple, 1);
		//map
		hades::console::SetProperty(editor_map_size, editor_map_size_default);
		//objects
		hades::console::SetProperty(editor_object_mock_size, editor_mock_size_default);
	}

	void RegisterObjectResources(hades::data::data_manager *data)
	{
		DefineObjectConsoleVars();

		data->register_resource_type("editor", resources::ParseEditor);
		data->register_resource_type("objects", resources::ParseObject);
		makeDefaultLayout(data);
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
		hades::resources::curve_default_value ParseValue<hades::data::UniqueId>(const YAML::Node &node)
		{
			hades::resources::curve_default_value out;
			try
			{
				auto s = node.as<hades::types::string>();
				auto u = hades::data::GetUid(s);
				if (u == hades::EmptyId)
					return out;

				out.set = true;
				out.value.emplace<hades::UniqueId>(u);
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
		hades::resources::curve_default_value ParseValueVector<hades::data::UniqueId>(const YAML::Node &node)
		{
			hades::resources::curve_default_value out;
			try
			{
				std::vector<hades::UniqueId> vec;
				for (auto v : node)
				{
					auto s = node.as<hades::types::string>();
					auto u = hades::data::GetUid(s);
					if (u != hades::EmptyId)
						vec.push_back(u);
				}

				out.set = true;
				out.value.emplace<std::vector<hades::UniqueId>>(vec);
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
				return ParseValue<hades::UniqueId>(node);
			case VariableType::VECTOR_INT:
				[[fallthrough]];
			case VariableType::VECTOR_OBJECT_REF:
				return ParseValueVector<hades::types::int32>(node);
			case VariableType::VECTOR_FLOAT:
				return ParseValueVector<float>(node);
			case VariableType::VECTOR_UNIQUE:
				return ParseValueVector<hades::UniqueId>(node);
			}

			return hades::resources::curve_default_value();
		}

		object::curve_obj ParseCurveObj(const YAML::Node &node, hades::data::data_manager *data)
		{
			object::curve_obj c;
			std::get<0>(c) = nullptr;

			static auto getPtr = [data](hades::types::string s)->hades::resources::curve* {
				if (s.empty())
					return nullptr;
				
				auto id = data->getUid(s);
				return data->get<hades::resources::curve>(id);
			};

			if (node.IsScalar())
			{
				auto id_str = node.as<hades::types::string>("");
				std::get<0>(c) = getPtr(id_str);
			}
			else if (node.IsSequence())
			{
				auto first = node[0];
				if (!first.IsDefined())
					return c;

				auto id_str = first.as<hades::types::string>("");
				std::get<0>(c) = getPtr(id_str);

				auto second = node[1];
				if (!second.IsDefined())
					return c;

				std::get<1>(c) = ParseDefaultValue(second, std::get<0>(c));
			}

			return c;
		}

		void ParseEditor(hades::data::UniqueId mod, const YAML::Node &node, hades::data::data_manager *data)
		{
			//editor:
			//    object-groups:
			//        <group-name>: object or [object1, object2, ...]
			//    object-mock-size: 8
			//    object-mock-colour:[r,g,b,a]
			//
			//    snap-to-grid: [-1, 0, 1, 2], [force-disabled, disabled, enabled, force-enabled]
			//    grid-min-size: default 8 or something, defines the small  
			//	  show-grid-settings: true // hides the toolbar buttons for modifying the grid
			//    show-grid: false // defines whether the grid should be turned on by default
			//    grid-colour: [r,g,b,a]
			//    grid-highlight: [r,g,b,a]
			//
			//    map-size:
			
			//use MakeColour to turn vectors into sf::color

			static const auto resource_type = "editor";
			static const auto editor_id = data->getUid(resource_type);

			if (!yaml_error(resource_type, "n/a", "n/a", "map", mod, node.IsMap()))
				return;

			//Parse Object variables

			auto groups = node["object-groups"];

			for (auto n : groups)
			{
				auto namenode = n.first;
				auto gname = namenode.as<hades::types::string>();
				
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
					auto obj_str = n.as<hades::types::string>();
					auto obj_id = hades::data::GetUid(obj_str);
					auto obj = hades::data::FindOrCreate<object>(obj_id, mod, data);
					g->obj_list.push_back(obj);
				};

				if (v.IsScalar())
					add_to_group(v);
				else if (v.IsSequence())
					for (auto o : v)
						add_to_group(o);
			}//for object groups

			auto mock_size = yaml_get_scalar<hades::types::int32>(node, "editor", "N/A", "object-mock-size", mod,
				*hades::console::GetInt(editor_object_mock_size, editor_mock_size_default));
			hades::console::SetProperty(editor_object_mock_size, mock_size);

			//Parse Grid Settings

			auto snap = node["snap-to-grid"];
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
					hades::console::SetProperty(editor_snaptogrid, intval);
			}

			// Get the minimum grid size for the editor(this should be the same value the game uses if it uses
			// grid based game logic
			hades::types::int32 current_grid_size = *hades::console::GetInt(editor_grid_size, editor_grid_default);
			auto grid_size = yaml_get_scalar<hades::types::int32>(node, "editor", "N/A", "grid-min-size", mod, current_grid_size);

			if (grid_size != current_grid_size)
			{
				hades::console::SetProperty(editor_grid_size, grid_size);
				hades::console::SetProperty(editor_grid_size_multiple, grid_size);
			}

			bool grid_enabled = yaml_get_scalar<bool>(node, "editor", "N/A", "show-grid", mod, 
				*hades::console::GetBool(editor_grid_enabled, true));
			hades::console::SetProperty(editor_grid_enabled, grid_enabled);

			//Parse Map Settings
			auto map_size = yaml_get_scalar<hades::types::int32>(node, "editor", "N/A", "map-size", mod, 
				*hades::console::GetInt(editor_map_size, editor_map_size_default));
			hades::console::SetProperty(editor_map_size, map_size);

			//========
			// editor settings object
			//========

			auto editor_obj = hades::data::FindOrCreate<editor>(editor_id, mod, data);

			//grid stuff
			editor_obj->show_grid_settings = yaml_get_scalar<bool>(node, "editor", "N/A", "show-grid-settings", mod, editor_obj->show_grid_settings);
			
			auto grid_colour = yaml_get_sequence<hades::types::int32>(node, "editor", "N/A", "grid-colour", mod);
			if (!grid_colour.empty())
				editor_obj->grid = hades::MakeColour(std::begin(grid_colour), std::end(grid_colour));

			auto grid_highlight = yaml_get_sequence<hades::types::int32>(node, "editor", "N/A", "grid-highlight", mod);
			if (!grid_highlight.empty())
				editor_obj->grid_highlight = hades::MakeColour(std::begin(grid_highlight), std::end(grid_highlight));

			//object settings
			auto object_mock = yaml_get_sequence<hades::types::int32>(node, "editor", "N/A", "object-mock-colour", mod);
			if (!object_mock.empty())
				editor_obj->object_mock_colour = hades::MakeColour(std::begin(object_mock), std::end(object_mock));

			//TODO:
			//toolbar icons
		}//parse editor

		void ParseObject(hades::data::UniqueId mod, const YAML::Node &node, hades::data::data_manager *data)
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
			//		client-systems : [system_id, ...]

			static const auto resource_type = "objects";

			if (!yaml_error(resource_type, "n/a", "n/a", "map", mod, node.IsMap()))
				return;

			for (auto n : node)
			{
				auto namenode = n.first;
				auto name = namenode.as<hades::types::string>();
				auto id = data->getUid(name);

				auto obj = hades::data::FindOrCreate<object>(id, mod, data);

				if (!obj)
					continue;

				Objects.push_back(obj);

				auto v = n.second;

				//=========================
				//get the icon used for the editor
				//=========================
				hades::data::UniqueId icon_id = hades::data::UniqueId::Zero;

				if (obj->editor_icon)
					icon_id = obj->editor_icon->id;

				icon_id = yaml_get_uid(v, resource_type, name, "editor-icon", mod, icon_id);

				if (icon_id != hades::data::UniqueId::Zero)
					obj->editor_icon = data->get<hades::resources::animation>(icon_id);

				//=========================
				//get the animation list used for the editor
				//=========================
				auto anim_list_ids = yaml_get_sequence<hades::types::string>(v, resource_type, name, "editor-anim", mod);
				
				auto anims = convert_string_to_resource<hades::resources::animation>(std::begin(anim_list_ids), std::end(anim_list_ids), data);
				std::move(std::begin(anims), std::end(anims), std::back_inserter(obj->editor_anims));

				//remove any duplicates
				std::sort(std::begin(obj->editor_anims), std::end(obj->editor_anims));
				obj->editor_anims.erase(std::unique(std::begin(obj->editor_anims), std::end(obj->editor_anims)), std::end(obj->editor_anims));
				//===============================
				//get the base objects
				//===============================

				auto base_list_ids = yaml_get_sequence<hades::types::string>(v, resource_type, name, "base", mod);
				auto base = convert_string_to_resource<object>(std::begin(base_list_ids), std::end(base_list_ids), data);
				std::copy(std::begin(base), std::end(base), std::back_inserter(obj->base));

				//remove dupes
				std::sort(std::begin(obj->base), std::end(obj->base));
				obj->base.erase(std::unique(std::begin(obj->base), std::end(obj->base)), std::end(obj->base));

				//=================
				//get the curves
				//=================

				auto curvesNode = v["curves"];
				if (yaml_error(resource_type, name, "curves", "map", mod, node.IsMap()))
				{
					for (auto c : curvesNode)
					{
						auto curve = ParseCurveObj(c, data);

						auto curve_ptr = std::get<0>(curve);

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
