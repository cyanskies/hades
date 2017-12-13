#include "Objects/resources.hpp"

#include "Hades/data_manager.hpp"

namespace objects
{
	namespace resources
	{
		void ParseObject(hades::data::UniqueId mod, const YAML::Node &node, hades::data::data_manager *data);
	}

	void RegisterObjectResources(hades::data::data_manager *data)
	{
		data->register_resource_type("objects", resources::ParseObject);		
	}

	namespace resources
	{
		object::curve_obj ParseCurveObj();

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
				std::copy(std::begin(anims), std::end(anims), std::back_inserter(obj->editor_anims));

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
