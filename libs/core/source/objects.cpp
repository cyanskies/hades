#include "hades/objects.hpp"

#include <algorithm>
#include <stack>

#include "hades/animation.hpp"
#include "hades/core_curves.hpp"
#include "hades/curve_extra.hpp"
#include "hades/data.hpp"
#include "hades/exceptions.hpp"
#include "hades/game_system.hpp"
#include "hades/level.hpp"
#include "hades/parser.hpp"
#include "hades/random.hpp"
#include "hades/utility.hpp"

using namespace std::string_view_literals;
using namespace std::string_literals;

static hades::unique_id editor_icon_id = hades::unique_zero;
static hades::unique_id editor_anim = hades::unique_zero;

namespace hades::resources
{
	// TODO: get rid off this, we cant have global refs like this anymore
	std::vector<resource_link<object>> all_objects{};

	static object::unloaded_curve get_curve_info2(data::data_manager& d, const unique_id parent, const data::parser_node& n)
	{
		// NOTE: n = sequence
		// eg:
		//	curves:
		//		-(n) [ curve_info[0], curve_info[1] ]
		// or
		//		-(n) curve_info
		const auto curve_info = n.get_children();

		const auto curve_id = curve_info[0]->to_scalar<unique_id>([&d](std::string_view s) {
			return d.get_uid(s);
		});

		if (!curve_id)
		{
			LOGWARNING("Invalid curve name while parsing object: " + d.get_as_string(parent) + ", name was: " + curve_info[0]->to_string());
			return {};
		}

		const auto curve_link = d.make_resource_link<curve>(curve_id, parent);
		auto out = object::unloaded_curve{ curve_link };
		const  auto size = std::size(curve_info);
		if (size > 1)
		{
			if(curve_info[1]->is_sequence())
				out.value = curve_info[1]->to_sequence<string>();
			else
				out.value = curve_info[1]->to_scalar<string>();
		}
		else
			out.value = std::monostate{};
		return out;
	}

	static void parse_objects(hades::unique_id mod, const data::parser_node& node, hades::data::data_manager& d)
	{
		//objects:
		//	thing:
		//		editor-anim : scalar animationId OR sequence [animId1, animId2, ...]
		//		animations: animation_group_id
		//		base : object_base OR [obj1, obj2, ...]
		//		curves:
		//			-[curve id, default value]
		//			-curve_id(no default value)
		//		systems: system_id or [system_id, ...]
		//		client-systems : system_id or [system_id, ...] // aka. render system

		constexpr auto resource_type = "objects"sv;
		const auto all_objects_id = d.get_uid("all-objects-list"sv);

		for (const auto& o : node.get_children())
		{
			const auto name = o->to_string();
			const auto id = d.get_uid(name);

			const auto obj = d.find_or_create<object>(id, mod, resource_type);

			// TODO: log something here
			if (!obj)
				continue;

			all_objects.push_back(d.make_resource_link<resources::object>(id, all_objects_id));

			using namespace data::parse_tools;

			//sprites used to represent to object in the editors map view
			const auto current_ids = resources::get_ids(obj->editor_anims);
			const auto animation_ids = get_unique_sequence(*o, "editor-anim"sv, current_ids);
			obj->editor_anims = animation_functions::make_resource_link(d, animation_ids, id);

			const auto anim_group_id = get_unique(*o, "animations"sv, unique_id::zero);
			if (anim_group_id)
				obj->animations = animation_group_functions::make_resource_link(d, anim_group_id, id);

			//base objects
			const auto current_base_ids = resources::get_ids(obj->base);
			const auto base_ids = get_unique_sequence(*o, "base"sv, current_base_ids);
			obj->base = d.make_resource_link<object>(base_ids, id);

			//curves
			const auto curves_node = o->get_child("curves"sv);
			if (curves_node)
			{
				for (const auto& c : curves_node->get_children())
				{
					auto unloaded = get_curve_info2(d, id, *c);
					if (unloaded.curve.is_linked())
						obj->curves.emplace_back(unloaded);
				}
			}

			obj->curves.shrink_to_fit();

			// check for duplicate curves
			// ie.
			//	- [health, 10]
			//	- health
			//	- [health, 100]
			//	
			//	Only one of these will survive, no telling which
			//	This indicates an error in the mod or data file.
			std::sort(begin(obj->curves), end(obj->curves), [](auto&& l, auto&& r) {
				return l.curve.id() < r.curve.id();
				});

			const auto iter = std::unique(begin(obj->curves), end(obj->curves), [](auto&& l, auto&& r) {
				return l.curve.id() == r.curve.id();
				});

			if (iter != end(obj->curves))
			{
				LOGWARNING("Duplicate curves provided while parsing object: " + d.get_as_string(id));
				obj->curves.erase(iter, end(obj->curves));
			}

			auto new_tags = merge_unique_sequence(*o, "tags"sv, std::move(obj->tags));
			remove_duplicates(new_tags);
			new_tags.shrink_to_fit();
			obj->tags = std::move(new_tags);

			//game systems
			auto current_system_ids = get_ids(obj->systems);
			const auto system_ids = merge_unique_sequence(*o, "systems"sv, std::move(current_system_ids));
			obj->systems = d.make_resource_link<system>(system_ids, id);

			//render systems
			auto current_render_system_ids = get_ids(obj->render_systems);
			const auto render_system_ids = merge_unique_sequence(*o, "client-systems"sv, std::move(current_render_system_ids));
			obj->render_systems = d.make_resource_link<render_system>(render_system_ids, id);
		}

		remove_duplicates(all_objects);
	}

	static curve_list get_all_curves(data::data_manager& d, resources::object& o)
	{
		auto out = curve_list{};

		using unloaded_curve = object::unloaded_curve;
		
		out.reserve(size(o.curves));
		// we dont want to include any curves that didnt get loaded, so we move them to the back
		const auto curve_end = std::partition(begin(o.curves), end(o.curves), std::mem_fn(&unloaded_curve::curve));

		std::transform(begin(o.curves), curve_end, back_inserter(out), [&d](const object::unloaded_curve& c) {
			assert(c.curve);
			return object::curve_obj{ c.curve.get(),
				curve_from_str(d, *c.curve.get(), c.value)};
			});

		for (auto& b : o.base)
		{
			assert(b && b->loaded);
			out.insert(end(out), begin(b->all_curves), end(b->all_curves));
		}

		return out;
	}

	static std::vector<resources::resource_link<resources::system>> get_all_systems(const resources::object& o)
	{
		auto out = std::vector<resources::resource_link<resources::system>>{};

		for (const auto& b : o.base)
		{
			assert(b && b->loaded);
			out.insert(end(out), begin(b->all_systems), end(b->all_systems));
		}

		out.insert(end(out), begin(o.systems), end(o.systems));
		return out;
	}

	static std::vector<resources::resource_link<resources::render_system>> get_all_render_systems(const resources::object& o)
	{
		auto out = std::vector<resources::resource_link<resources::render_system>>{};

		for (const auto& b : o.base)
		{
			assert(b && b->loaded);
			out.insert(end(out), begin(b->all_render_systems), end(b->all_render_systems));
		}

		out.insert(end(out), begin(o.render_systems), end(o.render_systems));
		return out;
	}

	static tag_list get_all_tags(const resources::object& o)
	{
		auto tags = tag_list{};

		for (auto& base : o.base)
		{
			assert(base && base->loaded);
			tags.insert(end(tags), begin(base->all_tags), end(base->all_tags));
		}

		tags.insert(end(tags), begin(o.tags), end(o.tags));
		return tags;
	}

}

namespace hades
{
	static curve_list unique_curves(curve_list);
}

namespace hades::resources
{
	static void load_objects(resource_type<object_t> &r, data::data_manager &d)
	{
		assert(dynamic_cast<object*>(&r));
		auto &o = static_cast<object&>(r);

		const auto name = d.get_as_string(o.id);

		for (auto& b : o.base)
		{
			if (!b->loaded)
				d.get<object>(b->id, b->mod);
		}

		namespace animf = animation_functions;
		namespace anim_groupf = animation_group_functions;

		for (const auto& a : o.editor_anims)
		{
			if (!animf::is_loaded(*a))
				animf::get_resource(d, animf::get_id(*a));
		}

		if (o.animations && !anim_groupf::is_loaded(*o.animations))
			anim_groupf::get_resource(d, anim_groupf::get_id(*o.animations));
		
		///collate the curves, tags and systems in all_* for fast lookup during gameplay
		o.all_curves = get_all_curves(d, o);
		o.all_curves = unique_curves(std::move(o.all_curves));
		o.all_curves.shrink_to_fit();
	
		o.all_systems = get_all_systems(o);
		remove_duplicates(o.all_systems);
		o.all_systems.shrink_to_fit();
		
		o.all_render_systems = get_all_render_systems(o);
		remove_duplicates(o.all_render_systems);
		o.all_render_systems.shrink_to_fit();
		
		o.all_tags = get_all_tags(o);
		remove_duplicates(o.all_tags);
		o.all_tags.shrink_to_fit();

		//all of the other resources used by objects are parse-only and don't require loading
		r.loaded = true;
	}

	object::object() : resource_type<object_t>(load_objects)
	{}
}

namespace hades::resources::object_functions
{
	unique_id get_id(const object& o) noexcept
	{
		return o.id;
	}

	const curve_list& get_all_curves(const resources::object& o) noexcept
	{
		assert(o.loaded);
		return o.all_curves;
	}

	const std::vector<resource_link<system>>& get_systems(const object& o) noexcept
	{
		assert(o.loaded);
		return o.all_systems;
	}

	const std::vector<resource_link<render_system>>& get_render_systems(const object& o) noexcept
	{
		assert(o.loaded);
		return o.all_render_systems;
	}

	const tag_list& get_tags(const object& o) noexcept
	{
		assert(o.loaded);
		return o.all_tags;
	}

	static object::unloaded_value to_unloaded_curve(const curve& c, curve_default_value v)
	{
		const auto def = reset_default_value(c);

		return std::visit([](auto&& t, auto&& v)->object::unloaded_value {
			using T = std::decay_t<decltype(t)>;
			using U = std::decay_t<decltype(v)>;
			if constexpr (std::is_same_v<U, std::monostate>)
			{
				return v;
			}
			else if constexpr (std::is_same_v<T, U>)
			{

				if constexpr (curve_types::is_vector_type_v<U>)
				{
					return std::vector{
						to_string(v.x),
						to_string(v.y)
					};
				}
				else if constexpr (curve_types::is_collection_type_v<U>)
				{
					auto out = std::vector<string>{};
					out.reserve(v.size());
					constexpr auto to_str = func_ref<string(typename U::value_type)>(to_string);
					std::transform(begin(v), end(v), back_inserter(out), to_str);
					return out;
				}
				else
					return to_string(v);
			}
			else
			{
				LOGERROR("Type mismatch while converting curve values"s);
				throw invalid_curve{ "Error converting curve values"s };
			}
			}, def, v);
	}

	void add_curve(object& o, unique_id i, curve_default_value v)
	{
		const auto c = data::get<resources::curve>(i);
		
		{
			// check object_curves
			auto iter = std::find_if(begin(o.curves), end(o.curves), [c](auto&& curve) {
				return c->id == curve.curve.id();
				});

			if (iter == end(o.curves))
			{
				auto value = to_unloaded_curve(*c, v);
				auto [d, lock] = data::detail::get_data_manager_exclusive_lock();
				std::ignore = lock;
				o.curves.emplace_back(object::unloaded_curve{
					d->make_resource_link<curve>(i, o.id),
					std::move(value)
					});
			}
		}

		if (!o.loaded)
			return;

		// check object all curves
		auto iter = std::find_if(begin(o.all_curves), end(o.all_curves), [other = c](auto&& c) {
			return other == c.curve;
			});

		if (iter == end(o.all_curves))
			o.all_curves.emplace_back(object::curve_obj{ c, std::move(v) });
		else
			iter->value = std::move(v);

		return;
	}

	bool has_curve(const object& o, const curve& c) noexcept
	{
		return std::any_of(begin(o.all_curves), end(o.all_curves), [&other = c](auto&& c){
			return c.curve == &other;
		});
	}

	bool has_curve(const object& o, const unique_id id) noexcept
	{
		return std::any_of(begin(o.all_curves), end(o.all_curves), [id](auto&& c) {
			return c.curve->id == id;
			});
	}

	void remove_curve(object& o, unique_id c)
	{
		{
			const auto iter = std::find_if(begin(o.curves), end(o.curves), [c](auto&& curve) {
				return c == curve.curve.id();
				});

			if (iter == end(o.curves))
				throw curve_not_found{ "Requested curve not found on object type: "s
					+ data::get_as_string(o.id) + ", curve was: "s
					+ data::get_as_string(c) };

			o.curves.erase(iter);
		}

		if (!o.loaded)
			return;

		// check for any new base values
		auto objects = std::stack<const object*>{};
		for (auto& object : o.base)
			objects.push(object.get());

		auto found_curves = curve_list{};

		auto [d, lock] = data::detail::get_data_manager_exclusive_lock();
		std::ignore = lock;
		while (!objects.empty())
		{
			const auto top = objects.top();

			const auto iter = std::find_if(begin(top->curves), end(top->curves), [c](auto&& curve) {
				return c == curve.curve.id();
				});

			if (iter != end(top->curves))
				found_curves.emplace_back(object::curve_obj{ iter->curve.get(), curve_from_str(*d, *iter->curve.get(), iter->value) });

			for (auto& obj : top->base)
				objects.push(obj.get());

			objects.pop();
		}

		found_curves = unique_curves(std::move(found_curves));
		o.all_curves.emplace_back(found_curves[0]);
		return;
	}

	curve_default_value get_curve(const object& o, const curve& c)
	{
		const auto iter = std::find_if(begin(o.all_curves), end(o.all_curves), [&other = c](auto&& c){
			return &other == c.curve;
		});

		if(iter == end(o.all_curves))
			throw curve_not_found{ "Requested curve not found on object type: "s 
				+ hades::data::get_as_string(o.id) + ", curve was: "s 
				+ hades::data::get_as_string(c.id) };

		if (auto& v = iter->value; is_set(v))
		{
			assert(is_curve_valid(*iter->curve, v));
			return v;
		}

		LOGWARNING("Curve wasn't set, curve: "s + data::get_as_string(c.id) + ", on object: "s + data::get_as_string(o.id));
		assert(hades::resources::is_set(c.default_value));
		return c.default_value;
	}

	const object::curve_obj& get_curve(const object& o, const unique_id id)
	{
		const auto iter = std::find_if(begin(o.all_curves), end(o.all_curves), [id](auto&& c) {
			return id == c.curve->id;
			});

		if (iter == end(o.all_curves))
			throw curve_not_found{ "Requested curve not found on object type: "s
				+ data::get_as_string(o.id) + ", curve was: "s
				+ data::get_as_string(id) };

		if (auto& v = iter->value; !is_set(v))
			throw curve_no_keyframes{ "Curve wasn't set, curve: "s +
				data::get_as_string(id) + ", on object: "s + data::get_as_string(o.id) };

		return *iter;
	}
}

namespace hades
{
	void register_objects(hades::data::data_manager &d)
	{
		editor_icon_id = d.get_uid("editor-icon"sv);
		editor_anim = d.get_uid("editor-anim"sv);

		register_animation_resource(d);
		register_curve_resource(d);
		register_core_curves(d);

		d.register_resource_type("objects"sv, resources::parse_objects);
	}

	using curve_obj = resources::object::curve_obj;

	//returns nullptr if the curve wasn't found
	//returns the curve with the value unset if it was found but not set
	//returns the requested curve plus it's value otherwise
	static curve_obj try_get_curve(const resources::object *o, const hades::resources::curve *c) noexcept
	{
		assert(o);
		assert(c);

		const auto iter = std::find_if(begin(o->all_curves), end(o->all_curves), [other = c](auto&& c) noexcept {
			return other == c.curve;
		});

		if (iter == end(o->all_curves))
			return {};

		return *iter;
	}

	object_instance make_instance(const resources::object *o) noexcept
	{
		return object_instance{ o };
	}

	object_save_instance make_save_instance(object_instance obj_instance)
	{
		auto obj = object_save_instance{};
		if (obj_instance.obj_type)
			obj.obj_type = obj_instance.obj_type;
		obj.id = obj_instance.id;
		obj.name_id = std::move(obj_instance.name_id);

		auto curve_list = get_all_curves(obj_instance);
		for (auto& [curve, val] : curve_list)
		{
			// only add curves that need to be saved
			if (!curve->save)
				continue;

			auto c = object_save_instance::saved_curve{};
			c.curve = curve;
			auto k = object_save_instance::saved_curve::saved_keyframe{};
			swap(k.value, val);
			c.keyframes.push_back(std::move(k));
			obj.curves.push_back(std::move(c));
		}

		return obj;
	}

	bool has_curve(const object_instance & o, const resources::curve & c) noexcept
	{
		//check object curve list
		if (std::any_of(std::begin(o.curves), std::end(o.curves), [c](auto &&curve) {
			return curve.curve == &c;
		}))
			return true;

		//check obj prototype
		if(o.obj_type)
			return resources::object_functions::has_curve(*o.obj_type, c);

		return false;
	}

	bool has_curve(const object_instance& o, const unique_id c) noexcept
	{
		//check object curve list
		if (std::any_of(std::begin(o.curves), std::end(o.curves), [c](auto&& curve) {
			return curve.curve->id == c;
			}))
		{
			return true;
		}

		//check obj prototype
		if (o.obj_type)
			return resources::object_functions::has_curve(*o.obj_type, c);

		return false;
	}

	curve_value get_curve(const object_instance &o, const hades::resources::curve &c)
	{
		//check the objects curve list
		for (const auto& cur : o.curves)
		{
			const auto& curve = cur.curve;
			const auto& v = cur.value;
			assert(curve);
			//if we have the right id and this
			if (curve->id == c.id && hades::resources::is_set(v))
				return v;
		}

		//check the object prototype
		assert(o.obj_type);
		const auto out = try_get_curve(o.obj_type, &c);
		if (const auto &[curve, value] = out; curve && hades::resources::is_set(value))
			return value;

		if (!has_curve(o, c))
			throw curve_not_found{"Tried to get_curve for curve: "s +  to_string(c.id) +
				" on an object that doesn't have it, object type: "s + to_string(o.obj_type->id) };

		//return the curves default
		assert(hades::resources::is_set(c.default_value));
		return c.default_value;
	}

	void set_curve(object_instance &o, const resources::curve &c, curve_value v)
	{
		assert(o.obj_type->loaded);
		const auto end = std::end(o.obj_type->all_curves);
		const auto iter = std::find_if(begin(o.obj_type->all_curves), end, [&other = c](auto&& c){
			return c.curve == &other;
		});

		if(iter == end)
			throw curve_not_found{ "Requested curve not found on object type: "s 
			+ hades::data::get_as_string(o.obj_type->id) + ", curve was: "s
			+ hades::data::get_as_string(c.id) };

		for (auto &[curve, value] : o.curves)
		{
			if (curve == &c)
			{
				assert(resources::is_curve_valid(c, v));
				value = std::move(v);
				return;
			}
		}

		if (c.keyframe_style == keyframe_style::const_t)
			throw curve_error{ "Tried to store const curve in a object instance." };

		o.curves.push_back({ iter->curve, std::move(v)});
		return;
	}

	void set_curve(object_instance& o, const unique_id i, curve_value v)
	{
		const auto c = data::get<resources::curve>(i);
		set_curve(o, *c, std::move(v));
	}

	static curve_list unique_curves(curve_list list)
	{
		//list should not contain any nullptr curves
		using curve = hades::resources::curve;
		using value = hades::resources::curve_default_value;

		std::stable_sort(begin(list), end(list), [](auto&& lhs, auto&& rhs)->bool {
			constexpr auto less = std::less<const resources::curve*>{};
			return less(lhs.curve, rhs.curve);
		});

		//for each unique curve, we want to keep the 
		//first with an assigned value or the last if their is no value
		const auto last = std::end(list);
		auto iter = std::begin(list);
		auto output = curve_list{};
		output.reserve(size(list));
		while (iter != last)
		{
			auto v = value{};
			const auto& c = iter->curve;
			assert(c);
			//while each item represents the same curve c
			//store the value if it is set
			//and then find the next different curve
			// or iterate to the next c
			do {
				auto &val = iter->value;
				if (hades::resources::is_set(val))
				{
					v = std::move(val);
					iter = std::find_if_not(iter, last, [c](const curve_obj &lhs) {
						return c == lhs.curve;
					});
				}
				else
					++iter;
			} while (iter != last && iter->curve == c);

			//store c and v in output
			if (!resources::is_set(v))
				v = resources::reset_default_value(*c);
			output.emplace_back(curve_obj{ c, std::move(v) });
		}

		return output;
	}

	curve_list get_all_curves(const object_instance &o)
	{
		curve_list curves = o.curves;
		if (o.obj_type)
		{
			const auto& obj_curves = resources::object_functions::get_all_curves(*o.obj_type);
			curves.insert(end(curves), begin(obj_curves), end(obj_curves));
		}
		return unique_curves(std::move(curves));
	}

	const hades::resources::animation *get_editor_icon(const resources::object &o)
	{
		namespace ag = resources::animation_group_functions;

		if (o.animations)
		{
			if (const auto icon =
				resources::animation_group_functions::get_animation(*o.animations, editor_icon_id);
				icon)
				return icon;
		}

		for (auto& b : o.base)
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
		// MSVC warning C26816: The pointer points to memory allocated on the stack
		// NOTE: it doesn't, it points to a animation resource allocated on the heap
		//	it's lifetime is managed by the data_manager
	}

	std::vector<const resources::animation*> get_editor_animations(const resources::object &o)
	{
		if (!o.editor_anims.empty())
		{
			auto out = std::vector<const resources::animation*>{};
			out.reserve(size(o.editor_anims));

			std::transform(begin(o.editor_anims), end(o.editor_anims), back_inserter(out),
				std::mem_fn(&resources::resource_link<resources::animation>::get));

			return out;
		}
		else if (o.animations)
		{
			if(auto anim = 
				resources::animation_group_functions::get_animation(*o.animations, editor_anim);
				anim)
				return { anim };
		}

		for (auto b : o.base)
		{
			assert(b);
			auto anim = get_editor_animations(*b);
			if (!anim.empty())
				return anim;
		}

		return std::vector<const resources::animation*>{};
	}

	std::vector<const resources::animation*> get_editor_animations(const object_instance & o)
	{
		assert(o.obj_type);
		return get_editor_animations(*o.obj_type);
	}

	const resources::animation* get_animation(const resources::object& o, unique_id id)
	{
		assert(o.animations);
		return resources::animation_group_functions::get_animation(*o.animations, id);
	}

	template<typename Object>
	static inline string get_name_impl(const Object &o)
	{
		try
		{
			const auto name_curve = get_name_curve();
			using resources::object_functions::get_curve;
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
		using resources::object_functions::get_curve;
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
	static inline resources::curve_types::unique get_vec_unique_impl(const Object &o, GetCurve get_vec_curve)
	{
		using group = resources::curve_types::unique;

		static_assert(std::is_invocable_r_v<const resources::curve*, GetCurve>);

		try
		{
			const auto curve = std::invoke(get_vec_curve);
			using resources::object_functions::get_curve;
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

	resources::curve_types::unique get_collision_groups(const object_instance &o)
	{
		return get_vec_unique_impl(o, get_collision_layer_curve);
	}

	resources::curve_types::unique get_collision_groups(const resources::object &o)
	{
		return get_vec_unique_impl(o, get_collision_layer_curve);
	}

	static void write_curve(data::writer& w, const curve_obj& c)
	{
		const auto& curve_ptr = c.curve;
		assert(curve_ptr);
		const auto name_str = to_string(curve_ptr->id);

		std::visit([&name_str, &w](auto&& v) {
			using T = std::decay_t<decltype(v)>;

			if constexpr (std::is_same_v<T, std::monostate>)
				throw logic_error{ "monostate poisoning"s };
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
		}, c.value);

		return;
	}

	constexpr auto obj_str = "objects"sv,
		obj_curves = "curves"sv,
		obj_type = "type"sv,
		obj_next = "next-id"sv,
		obj_name = "name"sv,
		obj_time = "create-time"sv;

	void serialise(const object_data& o, data::writer& w)
	{
		//root:
		//	next-id:
		//	objects:
		//		entity_id:
		//			obj-type:
		//			create-time:
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

			w.write(obj_time, ob.creation_time.time_since_epoch());

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
		//		create-time:
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

			const auto time = get_scalar(*o, obj_time, time_duration{}, duration_from_string);
			auto obj = object_instance{ object_type, id, name, time_point{time} };

			const auto curves_node = o->get_child(obj_curves);
			if (curves_node)
			{
				const auto curves = curves_node->get_children();
				for (const auto& c : curves)
				{
					const auto curve_id = c->to_scalar<unique_id>(data::make_uid);
					//TODO: log error?
					if (curve_id == unique_id::zero)
						continue;

					const auto curve_ptr = data::get<resources::curve>(curve_id);
					const auto curve_value = resources::curve_from_node(*curve_ptr, *c);

					assert(resources::is_set(curve_value));
					obj.curves.emplace_back(curve_obj{ curve_ptr, curve_value });
				}
			}

			out.objects.emplace_back(std::move(obj));
		}

		return out;
	}
}
