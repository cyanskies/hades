#include "Objects/resources.hpp"

#include "Hades/Data.hpp"
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
		template<class T>
		hades::resources::curve_default_value ParseValue(const YAML::Node &node)
		{
			hades::resources::curve_default_value out;
			try
			{
				out.set = true;
				std::get<T>(out.value) = node.as<T>();
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
				std::get<hades::UniqueId>(out.value) = u;
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
				std::get<std::vector<T>>(out.value) = vec;
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
				std::get<std::vector<hades::UniqueId>>(out.value) = vec;
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
				return ParseValue<hades::types::int32>(node);
			case VariableType::FLOAT:
				return ParseValue<float>(node);
			case VariableType::STRING:
				return ParseValue<hades::types::string>(node);
			case VariableType::UNIQUE:
				return ParseValue<hades::UniqueId>(node);
			case VariableType::VECTOR_INT:
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
