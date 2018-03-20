#include "Objects/Objects.hpp"

#include <algorithm>

#include "yaml-cpp/yaml.h"

#include "Hades/Data.hpp"
#include "Hades/data_system.hpp"
#include "Hades/exceptions.hpp"

namespace objects
{
	using curve_obj = resources::object::curve_obj;

	//returns nullptr if the curve wasn't found
	//returns the curve with the value unset if it was found but not set
	//returns the requested curve plus it's value otherwise
	curve_obj TryGetCurve(const resources::object *o, const hades::resources::curve *c)
	{
		assert(o && c);

		using curve_t = hades::resources::curve;
		const curve_t *curve_ptr = nullptr;
		for (auto cur : o->curves)
		{
			auto curve = std::get<const curve_t*>(cur);
			if (curve == c)
			{
				curve_ptr = curve;
				auto v = std::get<hades::resources::curve_default_value>(cur);
				if (v.set)
					return cur;
			}
		}

		for (auto obj : o->base)
		{
			assert(obj);
			auto ret = TryGetCurve(obj, c);
			auto curve = std::get<const curve_t*>(ret);
			if (curve)
			{
				curve_ptr = curve;
				if (std::get<hades::resources::curve_default_value>(ret).set)
					return ret;
			}
		}

		return std::make_tuple(curve_ptr, hades::resources::curve_default_value());
	}

	curve_value ValidVectorCurve(hades::resources::curve_default_value v)
	{
		using vector_int = hades::resources::curve_types::vector_int;
		assert(std::holds_alternative<vector_int>(v.value));

		//the provided value is valid
		if (auto vector = std::get<vector_int>(v.value); v.set && vector.size() >= 2)
			return v;

		//invalid value, override with a minimum valid value
		v.set = true;
		v.value = vector_int{0, 0};

		return v;
	}

	curve_value GetCurve(const object_info &o, const hades::resources::curve *c)
	{
		assert(c);
		for (auto cur : o.curves)
		{
			auto curve = std::get<const hades::resources::curve*>(cur);
			auto v = std::get<hades::resources::curve_default_value>(cur);
			assert(curve);
			//if we have the right id and this
			if (curve == c && v.set)
				return v;
		}

		assert(o.obj_type);
		auto out = TryGetCurve(o.obj_type, c);
		if (auto[curve, value] = out; curve && value.set)
			return value;

		assert(c->default_value.set);
		return c->default_value;
	}

	curve_value GetCurve(const resources::object *o, const hades::resources::curve *c)
	{
		assert(o && c);

		auto out = TryGetCurve(o, c);
		using curve_t = hades::resources::curve;
		if (std::get<const curve_t*>(out) == nullptr)
			throw curve_not_found("Requested curve not found on object type: " + hades::data::GetAsString(o->id)
				+ ", curve was: " + hades::data::GetAsString(c->id));

		if (auto v = std::get<hades::resources::curve_default_value>(out); v.set)
			return v;

		assert(c->default_value.set);
		return c->default_value;
	}

	void SetCurve(object_info &o, const hades::resources::curve *c, curve_value v)
	{
		for (auto &curve : o.curves)
		{
			if (std::get<const hades::resources::curve*>(curve) == c)
			{
				std::get<curve_value>(curve) = std::move(v);
				return;
			}
		}

		o.curves.push_back({ c, v });
	}

	curve_list UniqueCurves(curve_list list)
	{
		//list should not contain any nullptr curves
		using curve = hades::resources::curve;
		using value = hades::resources::curve_default_value;

		//place all curves of the same type adjacent to one another
		std::stable_sort(std::begin(list), std::end(list), [](const curve_obj &lhs, const curve_obj &rhs) {
			auto c1 = std::get<const curve*>(lhs), c2 = std::get<const curve*>(rhs);
			assert(c1 && c2);
			return c1->id < c2->id;
		});

		//remove any adjacent duplicates(for curves with the same type and value)
		std::unique(std::begin(list), std::end(list), [](const curve_obj &lhs, const curve_obj &rhs) {
			auto c1 = std::get<const curve*>(lhs), c2 = std::get<const curve*>(rhs);
			auto v1 = std::get<value>(lhs), v2 = std::get<value>(rhs);
			assert(c1 && c2);
			return c1->id == c2->id
				&& v1 == v2;
		});

		curve_list output;

		//for each unique curve, we want to keep the 
		//first with an assigned value or the last if their is no value
		auto first = std::begin(list);
		auto last = std::end(list);
		while (first != last)
		{
			value v;
			auto c = std::get<const curve*>(*first);
			//while each item represents the same curve c
			//store the value if it is set
			//and then find the next different curve
			// or iterate to the next c
			do {
				auto val = std::get<value>(*first);
				if (val.set)
				{
					v = val;
					first = std::find_if_not(first, last, [c](const curve_obj &lhs) {
						return c == std::get<const curve*>(lhs);
					});
				}
				else
					++first;
			} while (first != last && std::get<const curve*>(*first) == c);

			//store c and v in output
			output.push_back(std::make_tuple(c, v));
		}

		return output;
	}

	curve_list GetAllCurvesSimple(const resources::object *o)
	{
		assert(o);
		curve_list out;

		for (auto c : o->curves)
			out.push_back(c);

		for (auto obj : o->base)
		{
			assert(obj);
			auto curves = GetAllCurvesSimple(obj);
			std::move(std::begin(curves), std::end(curves), std::back_inserter(out));
		}

		return out;
	}

	curve_list GetAllCurves(const object_info &o)
	{
		curve_list output;

		for (auto c : o.curves)
			output.push_back(c);

		assert(o.obj_type);
		auto inherited_curves = GetAllCurvesSimple(o.obj_type);
		std::move(std::begin(inherited_curves), std::end(inherited_curves), std::back_inserter(output));

		return UniqueCurves(output);
	}
	
	curve_list GetAllCurves(const resources::object *o)
	{
		assert(o);
		auto curves = GetAllCurvesSimple(o);
		return UniqueCurves(curves);
	}

	const hades::resources::animation *GetEditorIcon(const resources::object *o)
	{
		assert(o);
		if (o->editor_icon)
			return o->editor_icon;

		for (auto b : o->base)
		{
			assert(b);
			auto icon = GetEditorIcon(b);
			if (icon)
				return icon;
		}

		return nullptr;
	}

	resources::object::animation_list GetEditorAnimations(const resources::object *o)
	{
		assert(o);
		if (!o->editor_anims.empty())
			return o->editor_anims;

		for (auto b : o->base)
		{
			assert(b);
			auto anim = GetEditorAnimations(b);
			if (!anim.empty())
				return anim;
		}

		return resources::object::animation_list();
	}

	constexpr auto level_str = "level",
		level_name = "name",
		level_desc = "description",
		level_width = "width",
		level_height = "height";

	constexpr auto obj_str = "objects",
		obj_curves = "curves",
		obj_type = "type",
		obj_next = "next_id",
		obj_name = level_name;

	void ReadObjectsFromYaml(const YAML::Node &root, level &l)
	{
		//yaml_root:
			//level:
				//width:
				//height:
				//title:
				//description:
				//background?
			//objects:
				//id:
					//object-type
					//name[opt]
					//curves:
						//[curve_id, value]
			//nextid: value

		//read level data
		const auto level_node = root[level_str];

		const auto name = yaml_get_scalar<hades::types::string>(level_node, level_str, level_str, "title", level_name);
		const auto desc = yaml_get_scalar<hades::types::string>(level_node, level_str, name, level_desc, "");
		l.map_x = yaml_get_scalar(level_node, level_str, name, level_width, 0);
		l.map_y = yaml_get_scalar(level_node, level_str, name, level_height, 0);
		l.next_id = yaml_get_scalar<hades::EntityId>(root, level_str, name, obj_next, hades::NO_ENTITY);

		//read objects
		auto object_list = root[obj_str];
		for (const auto &o : object_list)
		{
			object_info obj;
			const auto id_node = o.first;
			if (!id_node.IsDefined() || id_node.IsNull() || !id_node.IsScalar())
			{
				const auto message = "Id for object in level: " + hades::to_string(name) + ", is invalid or missing";
				LOGWARNING(message);
				continue;
			}

			obj.id = id_node.as<hades::EntityId>(hades::NO_ENTITY);

			const auto obj_node = o.second;
			obj.name = yaml_get_scalar<hades::types::string>(obj_node, obj_str, hades::to_string(obj.id), obj_name, "");
			const auto type_id = yaml_get_uid(obj_node, obj_str, obj.name, obj_type);

			if (type_id != hades::UniqueId::Zero)
			{
				const auto type_ptr = hades::data::Get<resources::object>(type_id);
				obj.obj_type = type_ptr;
			}

			const auto curves_node_root = obj_node[obj_curves];

			for (const auto &c : curves_node_root)
			{
				//TODO: error handling
				//log warnings here
				if (!c.IsSequence())
					continue;

				//and here
				if (c.size() < 2)
					continue;

				const auto curve_id_node = c[0];
				const auto curve_id_str = curve_id_node.as<hades::types::string>();
				//warning here
				if (curve_id_str.empty())
					continue;

				const auto id = hades::data::GetUid(curve_id_str);

				if (id == hades::UniqueId::Zero)
					continue;

				const auto curve_ptr = hades::data::Get<hades::resources::curve>(id);

				const auto curve_value_node = c[1];

				const auto value_str = curve_value_node.as<hades::types::string>();
				const auto value = hades::resources::StringToCurveValue(curve_ptr, value_str);

				obj.curves.push_back({ curve_ptr, value });
			}

			l.objects.push_back(obj);
		}
	}

	YAML::Emitter &WriteLevel(const level &l, YAML::Emitter &y)
	{
		//write the level info
		y << YAML::Key << level_str;
		y << YAML::Value << YAML::BeginMap;

		//name
		if (!l.name.empty())
		{
			y << YAML::Key << level_name;
			y << YAML::Value << l.name;
		}

		//description
		if (!l.description.empty())
		{
			y << YAML::Key << level_desc;
			y << YAML::Value << l.description;
		}

		//width
		y << YAML::Key << level_width;
		y << YAML::Value << l.map_x;

		//height:
		y << YAML::Key << level_height;
		y << YAML::Value << l.map_y;

		//end the level structure
		y << YAML::EndMap;
		return y;
	}

	YAML::Emitter &WriteObject(const object_info &o, YAML::Emitter &y)
	{
		y << YAML::Key << o.id;
		y << YAML::Value << YAML::BeginMap;

		//name
		if (!o.name.empty())
		{
			y << YAML::Key << obj_name;
			y << YAML::Value << o.name;
		}

		//type
		assert(o.obj_type);
		y << YAML::Key << obj_type;
		const auto type_id = hades::data::GetAsString(o.obj_type->id);
		y << YAML::Value << type_id;

		//curves
		if (!o.curves.empty())
		{
			y << YAML::Key << obj_curves;
			y << YAML::Value << YAML::BeginMap;

			for (const auto &[curve, value] : o.curves)
			{
				assert(curve);
				const auto curve_id = hades::data::GetAsString(curve->id);
				const auto value_str = hades::resources::CurveValueToString(value);

				y << YAML::Key << curve_id;
				y << YAML::Value << value_str;
			}

			y << YAML::EndMap;
		}

		return y << YAML::EndMap;
	}

	YAML::Emitter &WriteObjects(const level &l, YAML::Emitter &y)
	{
		//objects
			//id:
				//object-type
				//name[opt]
				//curves:
				//[curve_id, value]
		//next_id

		//start the list of objects
		if (!l.objects.empty())
		{
			y << YAML::Key << obj_str;
			y << YAML::Value << YAML::BeginMap;

			for (const auto &o : l.objects)
			{
				WriteObject(o, y);
			}

			//end the object list
			y << YAML::EndMap;
		}

		//add the next id value
		if (l.next_id != hades::NO_ENTITY)
		{
			y << YAML::Key << obj_next;
			y << YAML::Value << l.next_id;
		}

		return y;
	}

	YAML::Emitter &WriteObjectsToYaml(const level &l, YAML::Emitter& e)
	{
		WriteLevel(l, e);
		WriteObjects(l, e);
		return e;
	}
}