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

			auto resource_type = "objects";
		}
	}
}
