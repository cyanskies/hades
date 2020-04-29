#include "hades/objects.hpp"

#include <algorithm>
#include <stack>

#include "hades/animation.hpp"
#include "hades/core_curves.hpp"
#include "hades/curve_extra.hpp"
#include "hades/data.hpp"
#include "hades/exceptions.hpp"
#include "hades/level.hpp"
#include "hades/parser.hpp"
#include "hades/game_system.hpp"
#include "hades/utility.hpp"

namespace hades::resources
{
	std::vector<const object*> all_objects{};

	static std::tuple<const curve*, curve_default_value> get_curve_info(const data::parser_node &n)
	{
		//TODO: this is probably wrong
		// n == curve name and n.children are entries?
		const auto curve_info = n.get_children();
	
		if(curve_info.empty())
			return { nullptr, curve_default_value{} };

		using namespace data::parse_tools;

		const auto curve_id = curve_info[0]->to_scalar<unique_id>();
		
		try
		{
			const auto curve_ptr = data::get<curve>(curve_id);
			//NOTE: curve_ptr must be valid, get<> throws otherwise

			if (curve_info.size() == 2)
			{
				const auto value = curve_from_node(*curve_ptr, *curve_info[1]);
				return { curve_ptr, value };
			}

			return { curve_ptr, curve_ptr->default_value };
		}
		catch (const data::resource_null&)
		{
			const auto name = curve_info[0]->to_string();
			throw invalid_curve{ "\'" + name + "\' has not been registered as a resource name" };
		}
	}

	static void parse_objects(hades::unique_id mod, const data::parser_node &node, hades::data::data_manager &d)
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
		const auto& tags = *get_tags_curve();


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
			const auto editor_icon_id = get_unique(*o, "editor-icon"sv, unique_id::zero);
			if(editor_icon_id != unique_id::zero)
				obj->editor_icon = d.find_or_create<const animation>(editor_icon_id, mod);

			//sprites used to represent to object in the editors map view
			const auto current_ids = data::get_uid(obj->editor_anims);
			const auto animation_ids = get_unique_sequence(*o, "editor-anim"sv, current_ids);
			obj->editor_anims = d.find_or_create<const animation>(animation_ids, mod);

			//base objects
			const auto current_base_ids = data::get_uid(obj->base);
			const auto base_ids = get_unique_sequence(*o, "base"sv, current_base_ids);
			obj->base = d.find_or_create<const object>(base_ids, mod);

			//curves
			const auto curves_node = o->get_child("curves"sv);
			if (curves_node)
			{
				for (const auto &c : curves_node->get_children())
				{
					try
					{
						auto [curve, value] = get_curve_info(*c);

						//curve_info returns nullptr if 
						//c has no children
						if (curve)
							set_curve(*obj, *curve, std::move(value));
						//TODO: else: error
					}
					catch (const invalid_curve& c)
					{
						//get curve info will throw this if the curve type is unregistered
						// or its values are invalid
						using namespace std::string_literals;
						const auto msg = "Unable to add curve to object type: " + name + ", reason was: "s + c.what();
						LOGERROR(msg);
					}
					catch (const bad_conversion& c)
					{
						const auto msg = "Unable to add curve to object type: " + name + ", bad type conversion: " + c.what();
						LOGERROR(msg);
					}
				}
			}

			//tags
			auto current_tags = [obj, tags]()->curve_types::collection_unique {
				if (!has_curve(*obj, tags))
					return tag_list{};

				auto current_tags = get_curve(*obj, tags);
				assert(is_curve_valid(tags, current_tags));
				return std::get<curve_types::collection_unique>(std::move(current_tags));
			}();
			auto new_tags = merge_unique_sequence(*o, "tags"sv, std::move(current_tags));
			set_curve(*obj, tags, std::move(new_tags));

			//game systems
			auto current_system_ids = data::get_uid(obj->systems);
			const auto system_ids = merge_unique_sequence(*o, "systems"sv, std::move(current_system_ids));
			obj->systems = d.find_or_create<const system>(system_ids, mod);

			//render systems
			const auto current_render_system_ids = data::get_uid(obj->render_systems);
			const auto render_system_ids = merge_unique_sequence(*o, "client-systems"sv, current_system_ids);
			obj->render_systems = d.find_or_create<const render_system>(render_system_ids, mod);
		}

		remove_duplicates(all_objects);
	}

	static void load_objects(resource_type<object_t> &r, data::data_manager &d)
	{
		assert(dynamic_cast<object*>(&r));
		const auto &o = static_cast<object&>(r);

		//load all the referenced resources
		if (o.editor_icon && !o.editor_icon->loaded)
			d.get<animation>(o.editor_icon->id);

		for (const auto a : o.editor_anims)
		{
			if (!a->loaded)
				d.get<animation>(a->id);
		}

		//all of the other resources used by objects are parse-only and don't require loading
		r.loaded = true;
	}

	//TODO: define load object, needs to laod all resources that the object uses
	object::object() : resource_type<object_t>(load_objects)
	{}
}

namespace hades
{
	void register_objects(hades::data::data_manager &d)
	{
		using namespace std::string_view_literals;
		register_animation_resource(d);
		register_curve_resource(d);
		register_core_curves(d);

		d.register_resource_type("objects"sv, resources::parse_objects);
	}

	using curve_obj = resources::object::curve_obj;

	//TODO: FIXME: many of the functions below are recursive
	//			they should be changed to be iterative instead

	//returns nullptr if the curve wasn't found
	//returns the curve with the value unset if it was found but not set
	//returns the requested curve plus it's value otherwise
	//TODO: rename with correct naming scheme
	static curve_obj TryGetCurve(const resources::object *o, const hades::resources::curve *c)
	{
		assert(o);
		assert(c);

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
			}
		}

		return std::make_tuple(curve_ptr, hades::resources::curve_default_value{});
	}

	object_instance make_instance(const resources::object *o)
	{
		return object_instance{ o };
	}

	bool has_curve(const object_instance & o, const resources::curve & c)
	{
		//check object curve list
		if (std::any_of(std::begin(o.curves), std::end(o.curves), [&c](auto &&curve) {
			return std::get<const resources::curve*>(curve) == &c;
		}))
			return true;

		//check obj prototype
		if(o.obj_type)
			return has_curve(*o.obj_type, c);

		return false;
	}

	bool has_curve(const resources::object & o, const resources::curve & c)
	{
		return std::get<const resources::curve*>(TryGetCurve(&o, &c)) != nullptr;
	}

	curve_value get_curve(const object_instance &o, const hades::resources::curve &c)
	{
		//check the objects curve list
		for (const auto cur : o.curves)
		{
			const auto curve = std::get<const hades::resources::curve*>(cur);
			const auto v = std::get<hades::resources::curve_default_value>(cur);
			assert(curve);
			//if we have the right id and this
			if (curve->id == c.id && hades::resources::is_set(v))
				return v;
		}

		//check the object prototype
		if (o.obj_type)
		{
			const auto out = TryGetCurve(o.obj_type, &c);
			if (const auto &[curve, value] = out; curve && hades::resources::is_set(value))
				return value;
		}

		//return the curves default
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
				assert(resources::is_curve_valid(c, v));
				std::get<curve_value>(curve) = std::move(v);
				return;
			}
		}

		o.curves.push_back({ &c, std::move(v) });
	}

	void set_curve(object_instance& o, const unique_id i, curve_value v)
	{
		const auto c = data::get<resources::curve>(i);
		set_curve(o, *c, v);
	}

	void set_curve(resources::object& o, const hades::resources::curve& c, curve_value v)
	{
		const auto prev = std::find(begin(o.curves), end(o.curves), resources::object::curve_obj{ &c, v });

		//if the curve is already in the list, then just replace it's value
		//otherwise add it
		if (prev == end(o.curves))
			o.curves.emplace_back(&c, std::move(v));
		else
			*prev = { &c, std::move(v) };
	}

	static curve_list unique_curves(curve_list list)
	{
		//list should not contain any nullptr curves
		using curve = hades::resources::curve;
		using value = hades::resources::curve_default_value;

		std::stable_sort(std::begin(list), std::end(list), [](auto&& lhs, auto&& rhs) {
			return std::get<const curve*>(lhs)->id < std::get<const curve*>(rhs)->id;
		});

		//for each unique curve, we want to keep the 
		//first with an assigned value or the last if their is no value
		const auto last = std::end(list);
		auto iter = std::begin(list);
		auto output = curve_list{};
		while (iter != last)
		{
			auto v = value{};
			const auto c = std::get<const curve*>(*iter);
			assert(c);
			//while each item represents the same curve c
			//store the value if it is set
			//and then find the next different curve
			// or iterate to the next c
			do {
				auto &val = std::get<value>(*iter);
				if (hades::resources::is_set(val))
				{
					v = std::move(val);
					iter = std::find_if_not(iter, last, [c](const curve_obj &lhs) {
						return c == std::get<const curve*>(lhs);
					});
				}
				else
					++iter;
			} while (iter != last && std::get<const curve*>(*iter) == c);

			//store c and v in output
			if (!resources::is_set(v))
				v = resources::reset_default_value(*c);
			output.emplace_back(c, std::move(v));
		}

		return output;
	}

	static curve_list get_all_curves_iterative(const resources::object* o)
	{
		using object = resources::object;
		std::stack<const object*> objects{};
		curve_list out{};
		objects.emplace(o);

		do 
		{
			const auto cur = objects.top();
			objects.pop();
			out.insert(std::end(out), std::begin(cur->curves), std::end(cur->curves));

			if (!std::empty(cur->base))
			{
				auto iter = std::rbegin(cur->base);
				const auto end = std::rend(cur->base);
				do
				{
					objects.emplace(*iter);
				} while (++iter != end);
			}
		}while (!std::empty(objects));

		return out;
	}

	curve_list get_all_curves(const object_instance &o)
	{
		auto output = curve_list{};

		//curves from the instance
		for (auto c : o.curves)
			output.emplace_back(std::move(c));

		//get curves from the prototype
		if (o.obj_type)
		{
			auto inherited_curves = get_all_curves_iterative(o.obj_type);
			output.insert(std::end(output), std::make_move_iterator(std::begin(inherited_curves)),
				std::make_move_iterator(std::end(inherited_curves)));
		}

		return unique_curves(std::move(output));
	}
	
	curve_list get_all_curves(const resources::object &o)
	{
		auto curves = get_all_curves_iterative(&o);
		return unique_curves(std::move(curves));
	}

	std::vector<const resources::system*> get_systems(const resources::object& o)
	{
		auto out = std::vector<const resources::system*>{};

		for (const auto b : o.base)
		{
			const auto base_out = get_systems(*b);
			out.insert(std::end(out), std::begin(base_out), std::end(base_out));
		}

		out.insert(std::end(out), std::begin(o.systems), std::end(o.systems));
		return out;
	}

	std::vector<const resources::render_system*> get_render_systems(const resources::object& o)
	{
		auto out = std::vector<const resources::render_system*>{};

		for (const auto b : o.base)
		{
			const auto base_out = get_render_systems(*b); 
			out.insert(std::end(out), std::begin(base_out), std::end(base_out));
		}

		out.insert(std::end(out), std::begin(o.render_systems), std::end(o.render_systems));

		return out;
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

	const hades::resources::animation *get_random_animation(const object_instance & o)
	{
		const auto anims = get_editor_animations(o);
		if (anims.empty())
			return nullptr;

		const auto index = random(std::size_t{ 0 }, anims.size() - 1);

		return anims[index];
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

	resources::object::animation_list get_editor_animations(const object_instance & o)
	{
		assert(o.obj_type);
		return get_editor_animations(*o.obj_type);
	}

	template<typename Object>
	static inline string get_name_impl(const Object &o)
	{
		try
		{
			const auto name_curve = get_name_curve();
			const auto name = get_curve(o, *name_curve);
			assert(std::holds_alternative<string>(name));
			return std::get<string>(name);
		}
		catch (const curve_not_found&)
		{
			using namespace std::string_literals;
			return ""s;
		}
	}

	string get_name(const object_instance &o)
	{
		return get_name_impl(o);
	}

	string get_name(const resources::object &o)
	{
		return get_name_impl(o);
	}

	void set_name(object_instance &o, std::string_view name)
	{
		const auto name_curve = get_name_curve();
		set_curve(o, *name_curve, to_string(name));
	}

	template<typename Object, typename Func>
	static inline resources::curve_types::vec2_float get_vector_curve_impl(const Object &o, Func f)
	{
		const auto c = std::invoke(f);
		const auto value = get_curve(o, *c);
		assert(std::holds_alternative<resources::curve_types::vec2_float>(value));
		return std::get<resources::curve_types::vec2_float>(value);
	}

	resources::curve_types::vec2_float get_position(const object_instance &o)
	{
		return get_vector_curve_impl(o, get_position_curve);
	}

	resources::curve_types::vec2_float get_position(const resources::object &o)
	{
		return get_vector_curve_impl(o, get_position_curve);
	}

	void set_position(object_instance & o, resources::curve_types::vec2_float v)
	{
		const auto pos = get_position_curve();
		set_curve(o, *pos, v);
	}

	resources::curve_types::vec2_float get_size(const object_instance &o)
	{
		return get_vector_curve_impl(o, get_size_curve);
	}

	resources::curve_types::vec2_float get_size(const resources::object &o)
	{
		return get_vector_curve_impl(o, get_size_curve);
	}

	void set_size(object_instance &o, resources::curve_types::vec2_float v)
	{
		const auto siz = get_size_curve();
		set_curve(o, *siz, v);
	}

	template<typename Object, typename GetCurve>
	static inline resources::curve_types::collection_unique get_vec_unique_impl(const Object &o, GetCurve get_vec_curve)
	{
		using group = resources::curve_types::collection_unique;

		static_assert(std::is_invocable_r_v<const resources::curve*, GetCurve>);

		try
		{
			const auto curve = std::invoke(get_vec_curve);
			const auto name = get_curve(o, *curve);
			assert(std::holds_alternative<group>(name));
			return std::get<group>(name);
		}
		catch (const curve_not_found&)
		{
			using namespace std::string_literals;
			return group{};
		}
	}

	resources::curve_types::collection_unique get_collision_groups(const object_instance &o)
	{
		return get_vec_unique_impl(o, get_collision_group_curve);
	}

	resources::curve_types::collection_unique get_collision_groups(const resources::object &o)
	{
		return get_vec_unique_impl(o, get_collision_group_curve);
	}

	tag_list get_tags(const object_instance & o)
	{
		return get_vec_unique_impl(o, get_tags_curve);
	}

	tag_list get_tags(const resources::object & o)
	{
		return get_vec_unique_impl(o, get_tags_curve);
	}

	static void write_curve(data::writer& w, const curve_obj& c)
	{
		const auto curve_ptr = std::get<0>(c);
		assert(curve_ptr);
		const auto name_str = to_string(curve_ptr->id);

		std::visit([&name_str, &w](auto&& v) {
			using T = std::decay_t<decltype(v)>;

			if constexpr (std::is_same_v<T, std::monostate>)
				throw logic_error{"monostate poisoning"};
			else if constexpr (resources::curve_types::is_vector_type_v<T>)
			{
				w.start_sequence(name_str);
				w.write(v.x);
				w.write(v.y);
				w.end_sequence();
			}
			else if constexpr (resources::curve_types::is_collection_type_v<T>)
			{
				w.start_sequence(name_str);
				for(auto &elm : v)
					w.write(elm);
			}
			else
				w.write(name_str, v);

			return;
		}, std::get<1>(c));
		return;
	}

	using namespace std::string_view_literals;
	constexpr auto obj_str = "objects"sv,
		obj_curves = "curves"sv,
		obj_type = "type"sv,
		obj_next = "next_id"sv,
		obj_name = "name"sv;

	void serialise(const object_data& o, data::writer& w)
	{
		//root:
		//	next-id:
		//	objects:
		//		entity_id:
		//			obj-type:
		//			name:
		//			curves:
		//				id: [curve] //see write_curve()
		w.write(obj_next, o.next_id);

		if (std::empty(o.objects))
			return;

		//objects:
		w.start_map(obj_str);

		for (const auto& ob : o.objects)
		{
			//id:
			w.start_map(ob.id);

			//object-type:
			if (ob.obj_type)
				w.write(obj_type, ob.obj_type->id);

			//name:
			if (!ob.name_id.empty())
				w.write(obj_name, ob.name_id);

			if (!ob.curves.empty())
			{
				//curves:
				w.start_map(obj_curves);

				for (const auto& c : ob.curves)
					write_curve(w, c);

				//end curves:
				w.end_map();
			}

			//end id:
			w.end_map();
		}

		//end objects:
		w.end_map();
	}

	object_data deserialise_object_data(data::parser_node& n)
	{
		//root:
		//	next-id:
		//	objects:
		//		id:
		//		object-type:
		//		name[opt]:
		//		curves:
		//			curve_id: [curve] //see read_curve()

		auto out = object_data{};

		//read next id
		out.next_id = data::parse_tools::get_scalar(n, obj_next, out.next_id);

		//get objects
		const auto object_node = n.get_child(obj_str);
		if (!object_node)
			return out;

		const auto object_list = object_node->get_children();

		for (const auto& o : object_list)
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

			for (const auto& c : curves)
			{
				const auto curve_id = c->to_scalar<unique_id>();
				//TODO: log error?
					//TODO: same
				if (curve_id == unique_id::zero)
					continue;

				const auto curve_ptr = data::get<resources::curve>(curve_id);
				const auto curve_value = resources::curve_from_node(*curve_ptr, *c);

				if (resources::is_set(curve_value))
					obj.curves.emplace_back(curve_ptr, curve_value);
				else
					;//TODO: warning, unable to parse curve value
			}

			out.objects.emplace_back(obj);
		}

		return out;
	}
}