#include "hades/objects.hpp"

#include <algorithm>

#include "hades/animation.hpp"
#include "hades/curve_extra.hpp"
#include "hades/data.hpp"
#include "hades/exceptions.hpp"
#include "hades/level.hpp"
#include "hades/parser.hpp"
#include "hades/game_system.hpp"

namespace hades::resources
{
	std::vector<const object*> all_objects;

	std::tuple<const curve*, curve_default_value> get_curve_info(const data::parser_node &n, unique_id mod)
	{
		//TODO: this is probably wrong
		// n == curve name and n.children are entries?
		const auto curve_info = n.get_children();
	
		if(curve_info.empty())
			return { nullptr, curve_default_value{} };

		using namespace data::parse_tools;

		const auto curve_id = curve_info[0]->to_scalar<unique_id>();
		const auto curve_ptr = data::get<curve>(curve_id);

		if (curve_ptr && curve_info.size() > 2)
		{
			const auto value = curve_from_node(*curve_ptr, *curve_info[1]);
			return { curve_ptr, value };
		}

		return { curve_ptr, curve_default_value{} };		
	}

	void parse_objects(hades::unique_id mod, const data::parser_node &node, hades::data::data_manager &d)
	{
		//objects:
		//	thing:
		//		editor-icon : icon_anim
		//		editor-anim : scalar animationId OR sequence [animId1, animId2, ...]
		//		base : object_base OR [obj1, obj2, ...]
		//		curves:
		//			-[curve id, default value]
		//			-curve_id(no default value)
		//		systems: system_id or [system_id, ...]
		//		client-systems : system_id or [system_id, ...] // aka. render system

		using namespace std::string_view_literals;
		constexpr auto resource_type = "objects"sv;

		for (const auto &o : node.get_children())
		{
			const auto name = o->to_string();
			const auto id = d.get_uid(name);

			const auto obj = d.find_or_create<object>(id, mod);

			if (!obj)
				continue;

			all_objects.push_back(obj);

			using namespace data::parse_tools;

			//icon shown when selecting objects to place in the editor
			const auto editor_icon_id = get_scalar(*o, "editor-icon"sv, unique_id::zero);
			if(editor_icon_id != unique_id::zero)
				obj->editor_icon = d.find_or_create<const animation>(editor_icon_id, mod);

			//sprites used to represent to object in the editors map view
			const auto current_ids = data::get_uid(obj->editor_anims);
			const auto animation_ids = get_sequence(node, "editor-anim"sv, current_ids);
			obj->editor_anims = d.find_or_create<const animation>(animation_ids, mod);

			//base objects
			const auto current_base_ids = data::get_uid(obj->base);
			const auto base_ids = get_sequence(node, "base"sv, current_base_ids);
			obj->base = d.find_or_create<const object>(base_ids, mod);

			//curves
			const auto curves_node = o->get_child("curves"sv);
			if (curves_node)
			{
				for (const auto &c : curves_node->get_children())
				{
					const auto [curve, value] = get_curve_info(*c, mod);

					//curve_info returns nullptr if 
					//c has no children
					if (curve)
					{
						const auto prev = std::find(std::begin(obj->curves), std::end(obj->curves), object::curve_obj{ curve, value });
						
						//if the curve is already in the list, then just replace it's value
						//otherwise add it
						if (prev == std::end(obj->curves))
							obj->curves.emplace_back(curve, value);
						else
							*prev = { curve,value };
					}
					//TODO: else: error
				}
			}

			//game systems
			const auto current_system_ids = data::get_uid(obj->systems);
			const auto system_ids = get_sequence(*o, "systems"sv, current_system_ids);
			obj->systems = d.find_or_create<const system>(system_ids, mod);

			//render systems
			const auto current_render_system_ids = data::get_uid(obj->render_systems);
			const auto render_system_ids = get_sequence(*o, "client-systems"sv, current_system_ids);
			obj->render_systems = d.find_or_create<const render_system>(render_system_ids, mod);
		}
	}
}

namespace hades
{
	void register_objects(hades::data::data_manager &d)
	{
		using namespace std::string_view_literals;
		d.register_resource_type("objects"sv, resources::parse_objects);
	}

	using curve_obj = resources::object::curve_obj;

	//returns nullptr if the curve wasn't found
	//returns the curve with the value unset if it was found but not set
	//returns the requested curve plus it's value otherwise
	//TODO: rename with correct naming scheme
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
				if (hades::resources::is_set(v))
					return cur;

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
				if (hades::resources::is_set(std::get<hades::resources::curve_default_value>(ret)))
					return ret;
				return ret;
			}
		}

		return std::make_tuple(curve_ptr, hades::resources::curve_default_value());
	}

	curve_value valid_vector_curve(hades::resources::curve_default_value v)
	{
		using vector_int = hades::resources::curve_types::vector_int;
		assert(std::holds_alternative<vector_int>(v));

		//the provided value is valid
		if (auto vector = std::get<vector_int>(v); vector.size() >= 2)
			return v;

		//invalid value, override with a minimum valid value
		v = vector_int{0, 0};

		return v;
	}

	curve_value get_curve(const object_instance &o, const hades::resources::curve &c)
	{
		for (auto cur : o.curves)
		{
			auto curve = std::get<const hades::resources::curve*>(cur);
			auto v = std::get<hades::resources::curve_default_value>(cur);
			assert(curve);
			//if we have the right id and this
			if (curve->id == c.id && hades::resources::is_set(v))
				return v;
		}

		assert(o.obj_type);
		auto out = TryGetCurve(o.obj_type, &c);
		if (auto[curve, value] = out; curve && hades::resources::is_set(value))
			return value;

		assert(hades::resources::is_set(c.default_value));
		return c.default_value;
	}

	curve_value get_curve(const resources::object &o, const hades::resources::curve &c)
	{
		auto out = TryGetCurve(&o, &c);
		using curve_t = hades::resources::curve;
		if (std::get<const curve_t*>(out) == nullptr)
			throw curve_not_found{ "Requested curve not found on object type: " + hades::data::get_as_string(o.id)
				+ ", curve was: " + hades::data::get_as_string(c.id) };

		if (auto v = std::get<hades::resources::curve_default_value>(out); hades::resources::is_set(v))
			return v;

		assert(hades::resources::is_set(c.default_value));
		return c.default_value;
	}

	void set_curve(object_instance &o, const hades::resources::curve &c, curve_value v)
	{
		for (auto &curve : o.curves)
		{
			if (std::get<const hades::resources::curve*>(curve)->id == c.id)
			{
				std::get<curve_value>(curve) = std::move(v);
				return;
			}
		}

		o.curves.push_back({ &c, std::move(v) });
	}

	curve_list unique_curves(curve_list list)
	{
		//list should not contain any nullptr curves
		using curve = hades::resources::curve;
		using value = hades::resources::curve_default_value;

		const auto less = [](const curve_obj &lhs, const curve_obj &rhs) {
			auto c1 = std::get<const curve*>(lhs), c2 = std::get<const curve*>(rhs);
			assert(c1 && c2);
			return c1->id < c2->id;
		};

		const auto equal = [](const curve_obj &lhs, const curve_obj &rhs) {
			auto c1 = std::get<const curve*>(lhs), c2 = std::get<const curve*>(rhs);
			auto v1 = std::get<value>(lhs), v2 = std::get<value>(rhs);
			assert(c1 && c2);
			return c1->id == c2->id
				&& v1 == v2;
		};

		hades::remove_duplicates(list, less, equal);

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
				if (hades::resources::is_set(val))
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

	curve_list get_all_curves(const object_instance &o)
	{
		curve_list output;

		for (auto c : o.curves)
			output.push_back(c);

		assert(o.obj_type);
		auto inherited_curves = GetAllCurvesSimple(o.obj_type);
		std::move(std::begin(inherited_curves), std::end(inherited_curves), std::back_inserter(output));

		return unique_curves(output);
	}
	
	curve_list get_all_curves(const resources::object &o)
	{
		auto curves = GetAllCurvesSimple(&o);
		return unique_curves(curves);
	}

	const hades::resources::animation *get_editor_icon(const resources::object &o)
	{
		if (o.editor_icon)
			return o.editor_icon;

		for (auto b : o.base)
		{
			assert(b);
			auto icon = get_editor_icon(*b);
			if (icon)
				return icon;
		}

		return nullptr;
	}

	resources::object::animation_list get_editor_animations(const resources::object &o)
	{
		if (!o.editor_anims.empty())
			return o.editor_anims;

		for (auto b : o.base)
		{
			assert(b);
			auto anim = get_editor_animations(*b);
			if (!anim.empty())
				return anim;
		}

		return resources::object::animation_list{};
	}

	using namespace std::string_view_literals;
	constexpr auto obj_str = "objects"sv,
		obj_curves = "curves"sv,
		obj_type = "type"sv,
		obj_next = "next_id"sv,
		obj_name = "name"sv;

	void write_objects_from_level(const hades::level &l, data::writer &w)
	{
		//yaml_root:
			//next-id:
			//objects:
				//id:
					//object-type
					//name[opt]
					//curves:
						//curve_id: value

		//next-id:
		w.write(obj_next, l.next_id);

		if (l.objects.empty())
			return;

		//objects:
		w.start_map(obj_str);

		for (const auto &o : l.objects)
		{
			//id:
			w.start_map(o.id);

			//	object-type:
			if(o.obj_type)
				w.write(obj_type, o.obj_type->id);

			//	name:
			if (!o.name.empty())
				w.write(obj_name, o.name);

			if (!o.curves.empty())
			{
				//	curves:
				w.start_map(obj_curves);
				
				for (const auto &c : o.curves)
				{
					const auto curve_ptr = std::get<0>(c);
					assert(curve_ptr);
					w.write(to_string(curve_ptr->id), curve_to_string(*curve_ptr, std::get<1>(c)));
				}

				//end curves:
				w.end_map();
			}

			//end id:
			w.end_map();
		}

		//end objects:
		w.end_map();
	}
	
	void read_objects_into_level(const data::parser_node &n, hades::level &l)
	{
		//yaml_root:
			//next-id
			//objects:
				//id:
					//object-type
					//name[opt]
					//curves:
						//[curve_id, value]

		//read next id
		l.next_id = data::parse_tools::get_scalar(n, obj_next, l.next_id);

		//get objects
		const auto object_node = n.get_child(obj_str);
		const auto object_list = object_node->get_children();

		for (const auto &o : object_list)
		{
			const auto id = o->to_scalar<entity_id>();

			using namespace data::parse_tools;
			const auto name = get_scalar(*o, obj_name, string{});

			const auto object_type_id = get_unique(*o, obj_type, unique_id::zero);
			const auto object_type = [object_type_id]()->const resources::object* {
				if (object_type_id == unique_id::zero)
					return nullptr;
				else
					return data::get<resources::object>(object_type_id);
			}();

			auto obj = object_instance{ object_type, id, name };

			const auto curves_node = o->get_child(obj_curves);
			const auto curves = curves_node->get_children();

			for (const auto &c : curves)
			{
				const auto curve_info = c->get_children();
				//TODO: log error?
				if (curve_info.size() < 2)
					continue;

				const auto curve_id = curve_info[0]->to_scalar<unique_id>([](std::string_view s) {
					return data::get_uid(s);
				});

				//TODO: same
				if (curve_id == unique_id::zero)
					continue;

				const auto curve_ptr = data::get<resources::curve>(curve_id);
				const auto curve_value = resources::curve_from_node(*curve_ptr, *curve_info[1]);

				if (resources::is_set(curve_value))
					obj.curves.emplace_back(curve_ptr, curve_value);
				else
					;//TODO: warning, unable to parse curve value
			}

			l.objects.emplace_back(obj);
		}
	}
}