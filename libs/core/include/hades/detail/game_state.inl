#include "hades/game_state.hpp"

#include "hades/for_each_tuple.hpp"

namespace hades::state_api
{
	namespace detail
	{
		template<template<typename> typename CurveType, typename T>
		static inline void create_object_property(game_obj& object, unique_id curve_id,
			game_state& state, std::vector<object_save_instance::saved_curve::saved_keyframe> value)
		{
			//add the curve and value into the game_state database
			//then record a ptr to the data in the game object
			auto& colony = std::get<game_state::data_colony<CurveType, T>>(state.state_data);
			auto curve = CurveType<T>{};

			if constexpr (std::is_same_v<CurveType<T>, const_curve<T>>)
			{
				assert(!std::empty(value));
				curve.set(std::move(std::get<T>(value[0].value)));
			}
			else
			{
				for (const auto& [time, frame_value] : value)
					curve.add_keyframe(time, std::move(std::get<T>(frame_value)));
			}

			auto iter = colony.insert(state_field<CurveType<T>>{
				object.id, curve_id, std::move(curve) 
			});

			using entry_type = typename game_obj::var_entry;
			object.object_variables.push_back(entry_type{
				curve_id,
				static_cast<void*>(&*iter),
				get_curve_info<CurveType, T>()
			});
			return;
		}

		struct make_object_visitor
		{
			const resources::curve* c;
			game_state& s;
			game_obj& obj;
			std::vector<object_save_instance::saved_curve::saved_keyframe> keyframes;

			template<typename Ty, std::enable_if_t<curve_types::is_linear_interpable_v<std::decay_t<Ty>>, int> = 0>
			void operator()(Ty& v)
			{
				//add the curve and value into the game_state database
				//then record a ptr to the data in the game object
				using T = std::decay_t<decltype(v)>;
				switch (c->keyframe_style)
				{
				case keyframe_style::const_t:
					return create_object_property<const_curve, T>(obj, c->id, s, std::move(keyframes));
				case keyframe_style::linear:
					return create_object_property<linear_curve, T>(obj, c->id, s, std::move(keyframes));
				case keyframe_style::pulse:
					return create_object_property<pulse_curve, T>(obj, c->id, s, std::move(keyframes));
				case keyframe_style::step:
					return create_object_property<step_curve, T>(obj, c->id, s, std::move(keyframes));
				case keyframe_style::end:
					throw hades::logic_error{ "invalid keyframe style" };
				}
			}

			template<typename Ty, std::enable_if_t<!curve_types::is_linear_interpable_v<std::decay_t<Ty>>
				&& !std::is_same_v<std::decay_t<Ty>, std::monostate>, int> = 0>
			void operator()(Ty& v)
			{
				//add the curve and value into the game_state database
				//then record a ptr to the data in the game object
				using T = std::decay_t<decltype(v)>;
				switch (c->keyframe_style)
				{
				case keyframe_style::const_t:
					return create_object_property<const_curve, T>(obj, c->id, s, std::move(keyframes));
				case keyframe_style::linear:
					throw hades::logic_error{ "linear curve on wrong type" };
				case keyframe_style::pulse:
					return create_object_property<pulse_curve, T>(obj, c->id, s, std::move(keyframes));
				case keyframe_style::step:
					return create_object_property<step_curve, T>(obj, c->id, s, std::move(keyframes));
				case keyframe_style::end:
					throw hades::logic_error{ "invalid keyframe style" };
				}
			}

			template<typename Ty, std::enable_if_t<std::is_same_v<std::decay_t<Ty>, std::monostate>, int> = 0> // uncalled function to keep std::variant happy
			void operator()(Ty&) { assert(false); throw logic_error{ "monostate in game_state.inl" }; return; }
		};

		template<typename GameSystem>
		inline object_ref make_object_impl(const object_instance& o, time_point t, game_state& s, extra_state<GameSystem>& e)
		{
			// give this instance a new unique id if  it doesn't have one
			const auto id = o.id == bad_entity ? increment(s.next_id) : o.id;

			// insert always returns a valid ptr
			auto obj = e.objects.insert(game_obj{ id, o.obj_type });
			assert(obj);

			auto curves = get_all_curves(o);

			for (auto& [c, v] : curves)
			{
				assert(c);
				assert(!v.valueless_by_exception());
				assert(resources::is_set(v));
				std::visit(detail::make_object_visitor{ c, s,
					*obj, std::vector{ object_save_instance::saved_curve::saved_keyframe{t, v} } }, v);
			}

			assert(size(curves) == size(obj->object_variables));

			if (!empty(o.name_id))
				name_object(o.name_id, { id, obj }, t, s);

			return { id, obj };
		}

		template<typename GameSystem>
		inline object_ref make_object_impl(const object_save_instance& o, time_point t, game_state& s, extra_state<GameSystem>& e)
		{
			// give this instance a new unique id if it doesn't have one
			const auto id = o.id == bad_entity ? increment(s.next_id) : o.id;

			// insert always returns a valid ptr
			auto obj = e.objects.insert(game_obj{ id, o.obj_type });
			assert(obj);

			//add the saved curves
			for (auto& [c, v] : o.curves)
			{
				assert(c);
				assert(!std::empty(v));

				std::visit(detail::make_object_visitor{ c, s, *obj, v }, v[0].value);
			}

			assert(size(o.curves) == size(obj->object_variables));

			//add the object curves that weren't saved
			//list of curve ids
			auto ids = std::vector<unique_id>{};
			ids.reserve(size(o.curves));
			std::transform(begin(o.curves), end(o.curves), back_inserter(ids), [](auto&& saved_curve) {
				return saved_curve.curve->id;
			});

			std::sort(begin(ids), end(ids));

			auto base_curves = get_all_curves(*o.obj_type);
			for (auto& [c, v] : base_curves)
			{
				//if the id is in the saved list, then don't restore it
				if (std::binary_search(begin(ids), end(ids), c->id))
					continue;

				auto keyframes = std::vector<object_save_instance::saved_curve::saved_keyframe>{};
				keyframes.push_back({ time_point{}, std::move(v) });
				std::visit(detail::make_object_visitor{c, s, *obj, keyframes}, keyframes[0].value);
			}

			if (!empty(o.name_id))
				name_object(o.name_id, { id, obj }, t, s);

			return { id, obj };
		}
	}

	namespace loading
	{
		template<typename GameSystem>
		object_ref restore_object(const object_save_instance& o, game_state& s, extra_state<GameSystem>& e)
		{
			assert(o.obj_type);
			assert(o.id != bad_entity);
			assert(o.id < s.next_id);
			
			if (e.objects.find(o.id) != nullptr)
				throw object_id_collision{ "tried to restore an object that has a matching id to a currently living object" };
			const auto obj = detail::make_object_impl(o, o.creation_time, s, e);
			s.object_creation_time[o.id] = o.creation_time;
			s.object_destruction_time[o.id] = o.destruction_time;

			for (const auto sys : get_systems(*o.obj_type))
				e.systems.attach_system_from_load(obj, sys->id);

			//add name to name map
			auto id_curve = step_curve<object_ref>{};
			id_curve.add_keyframe(o.creation_time, obj);
			s.names.insert_or_assign(o.name_id, std::move(id_curve));
			
			return obj;
		}
	}

	template<typename GameSystem>
	inline object_ref make_object(const object_instance& o, const time_point t, game_state& s, extra_state<GameSystem>& e)
	{
		if (o.id != bad_entity)
			throw game_state_error{ "tried to create object with preset id" };
		auto obj = detail::make_object_impl(o, t, s, e);
		s.object_creation_time[obj.id] = t;
		for (const auto sys : get_systems(*o.obj_type))
			e.systems.attach_system(obj, sys->id);

		return obj;
	}

	namespace detail
	{
		using curve_info_t = std::pair<keyframe_style, curve_variable_type>;

		template<template<typename> typename CurveType, typename T>
		constexpr auto linear_compat = (std::is_same_v<CurveType<float>, linear_curve<float>>
				&& curve_types::is_linear_interpable_v<T>)
				|| !std::is_same_v<CurveType<float>, linear_curve<float>>;

		template<template<typename> typename CurveType, typename T, typename Func,
			typename std::enable_if_t<!linear_compat<CurveType, T>, int> = 0>
		constexpr void do_call_with_curve_type(Func&) noexcept
		{
			assert(false);
			return;
		}

		template<template<typename> typename CurveType, typename T, typename Func,
			typename std::enable_if_t<linear_compat<CurveType, T>, int> = 0>
		void do_call_with_curve_type(Func& f)
		{
			f.operator()<CurveType, T>();
			return;
		}

		template<template<typename> typename CurveType, typename Func>
		void call_with_curve_type(curve_variable_type curve_info, Func& f)
		{
			using v = curve_variable_type;
			using namespace curve_types;
			switch (curve_info)
			{
			case v::int_t:
				return do_call_with_curve_type<CurveType, int_t>(f);
			case v::float_t:
				return do_call_with_curve_type<CurveType, float_t>(f);
			case v::vec2_float:
				return do_call_with_curve_type<CurveType, vec2_float>(f);
			case v::bool_t:
				return do_call_with_curve_type<CurveType, bool_t>(f);
			case v::string:
				return do_call_with_curve_type<CurveType, curve_types::string>(f);
			case v::object_ref:
				return do_call_with_curve_type<CurveType, curve_types::object_ref>(f);
			case v::unique:
				return do_call_with_curve_type<CurveType, unique>(f);
			case v::colour:
				return do_call_with_curve_type<CurveType, colour>(f);
			case v::time_d:
				return do_call_with_curve_type<CurveType, time_d>(f);
			case v::collection_int:
				return do_call_with_curve_type<CurveType, collection_int>(f);
			case v::collection_float:
				return do_call_with_curve_type<CurveType, collection_float>(f);
			case v::collection_object_ref:
				return do_call_with_curve_type<CurveType, collection_object_ref>(f);
			case v::collection_unique:
				return do_call_with_curve_type<CurveType, collection_unique>(f);			
			case v::collection_colour:
				return do_call_with_curve_type<CurveType, collection_colour>(f);
			case v::collection_time_d:
				return do_call_with_curve_type<CurveType, collection_time_d>(f);
			case v::error:
				;
			}
			throw logic_error{ "invalid curve variable type" };
		}

		template<typename Func>
		void call_with_curve_info(curve_info_t curve_info, Func& f)
		{
			using k = keyframe_style;
			switch (curve_info.first)
			{
			case k::const_t:
				return call_with_curve_type<const_curve>(curve_info.second, f);
			case k::linear:
				return call_with_curve_type<linear_curve>(curve_info.second, f);
			case k::pulse:
				return call_with_curve_type<pulse_curve>(curve_info.second, f);
			case k::step:
				return call_with_curve_type<step_curve>(curve_info.second, f);
			case k::end:
				;
			}
			throw logic_error{ "invalid keyframe style" };
		}

		class object_copy_functor
		{
		public:
			game_obj* obj;
			game_state& state;
			time_point t;
			game_obj::var_entry::state_field_ptr var;

			template<template <typename> typename CurveType, typename VarType>
			void operator()()
			{
				auto state_var = static_cast<state_field<CurveType<VarType>>*>(var);
				copy_curve(state_var->id, state_var->data);
				return;
			}

			//copy_curve
			// copies the param at the current time point to the object
			template<typename T>
			void copy_curve(unique_id id, const const_curve<T>& c)
			{
				using saved_keyframes = std::vector<object_save_instance::saved_curve::saved_keyframe>;
				auto frames = saved_keyframes{};
				frames.push_back({ time_point::min(), c.get() });
				detail::create_object_property<const_curve, T>(*obj, id, state, std::move(frames));
				return;
			}

			//func for linear and step curves
			template<template<typename> typename CurveType, typename T,
				std::enable_if_t<std::is_same_v<CurveType<T>, linear_curve<T>>
				|| std::is_same_v<CurveType<T>, step_curve<T>>, int> = 0>
				void copy_curve(unique_id id, const CurveType<T>& c)
			{
				using saved_keyframes = std::vector<object_save_instance::saved_curve::saved_keyframe>;
				auto frames = saved_keyframes{};
				frames.push_back({ t, c.get(t) });
				detail::create_object_property<CurveType, T>(*obj, id, state, std::move(frames));
				return;
			}

			template<typename T>
			void copy_curve(unique_id id, const pulse_curve<T>& c)
			{
				using saved_keyframes = std::vector<object_save_instance::saved_curve::saved_keyframe>;
				const auto pulses = c.get(t);
				auto frames = saved_keyframes{};
				frames.reserve(2u);
				frames.push_back({ pulses.first.time, pulses.first.value });
				if (pulses.second != pulse_curve<T>::bad_keyframe)
					frames.push_back({ pulses.second.time, pulses.second.value });

				detail::create_object_property<pulse_curve, T>(*obj, id, state, std::move(frames));
				return;
			}
		};
	}

	template<typename GameSystem>
	inline object_ref clone_object(const game_obj& obj, const time_point t, game_state& state, extra_state<GameSystem>& extra)
	{
		const auto id = increment(state.next_id);
		auto new_obj = extra.objects.insert(game_obj{ id, obj.object_type });

		//copy all object properties
		for (const auto& var_entry : obj.object_variables)
		{
			auto functor =
				detail::object_copy_functor{ new_obj, state, t, var_entry.var };
			detail::call_with_curve_info(var_entry.info, functor);
		}

		state.object_creation_time[id] = t;
		const auto ref = object_ref{ id, new_obj };
		//attach all systems
		for (const auto sys : get_systems(*obj.object_type))
			extra.systems.attach_system(ref, sys->id);

		return ref;
	}

	template<typename GameSystem>
	void destroy_object(object_ref o, time_point t, game_state& s, extra_state<GameSystem>& e)
	{
		e.systems.detach_all(o);
		s.object_destruction_time.insert({ o.id, t });
		return;
	}

	namespace detail
	{
		class erase_object_visitor 
		{
		public:
			game_state& s;
			void* var;

			template<template<typename> typename CurveType, typename T>
			void operator()()
			{
				using namespace std::string_literals;
				const auto entry = static_cast<state_field<CurveType<T>>*>(var);
				//TODO: search the colony for the target vars more efficiently
				auto& list = std::get<game_state::data_colony<CurveType, T>>(s.state_data);
				const auto iter = list.get_iterator(entry);
				if (iter == end(list))
				{
					throw state_api::object_property_not_found{ "expected property not found while erasing object: "s
						+ to_string(entry->object) + "; property: "s + to_string(entry->id)};
				}
				else
					list.erase(iter);
				return;
			}
		};
	}

	template<typename GameSystem>
	void erase_object(game_obj& o, game_state& s, extra_state<GameSystem>& e)
	{
		for (const auto& entry : o.object_variables)
		{
			auto info = entry.info;
			auto visitor = detail::erase_object_visitor{ s, entry.var };
			detail::call_with_curve_info(info, visitor);
		}
		o.object_variables.clear();

		o.id = bad_entity;
		e.objects.erase(&o);
		return;
	}

	template<typename GameSystem>
	object_ref get_object_ref(std::string_view s, time_point t, game_state& g, extra_state<GameSystem>& e) noexcept
	{
		//find and entry for this name
		auto obj = g.names.find(to_string(s));
		if (obj == end(g.names))
		{
			LOGWARNING("tried to find named object that doesn't exist, name was: " + to_string(s));
			return object_ref{};
		}

		//check what object is tied to the name at time_point
		object_ref ob_ref = obj->second.get(t);
		if (ob_ref == object_ref{})
			return object_ref{};

		auto ptr = get_object_ptr(ob_ref, e);
		if (ptr == nullptr)
		{
			obj->second.add_keyframe(t, object_ref{});
			return object_ref{};
		}

		ob_ref.ptr = ptr;
		return ob_ref;
	}

	template<typename GameSystem>
	game_obj& get_object(object_ref& o, extra_state<GameSystem>& e)
	{
		if (o.ptr == nullptr)
		{
			o.ptr = e.objects.find(o.id);

			//entity is gone, goodbye
			if (o.ptr == nullptr)
				throw object_stale_error{ "stale object ref, the object is no longer with us" };

			return *o.ptr;
		}

		if (o.ptr->id == bad_entity)
		{
			o.ptr = nullptr;
			throw object_stale_error{ "dead object" };
		}

		if (o.id != o.ptr->id)
		{
			o.ptr = nullptr;
			throw object_stale_error{ "stale object ref, the ptr has been resused for a new object" };
		}

		return *o.ptr;
	}

	template<typename GameSystem>
	game_obj* get_object_ptr(object_ref& o, extra_state<GameSystem>& e) noexcept
	{
		const auto good_obj = o.ptr && o.ptr->id == o.id;
		if (good_obj)
			return o.ptr;

		auto obj_ptr = e.objects.find(o.id);

		o.ptr = obj_ptr;
		return obj_ptr;
	}

	template<typename GameSystem>
	const game_obj* get_object_ptr(const object_ref& o, const extra_state<GameSystem>& e) noexcept
	{
		const auto good_obj = o.ptr && o.ptr->id == o.id;
		if (good_obj)
			return o.ptr;

		return e.objects.find(o.id);
	}

	namespace detail
	{
		template<template<typename> typename CurveType, typename T, typename GameObj>
		inline CurveType<T>* get_object_property_ptr(GameObj& g, const variable_id v) noexcept
		{
			static_assert(curve_types::is_curve_type_v<T> && (curve_types::is_linear_interpable_v<T> || !std::is_same_v<CurveType<T>, linear_curve<T>>),
				"T must be one of the curve types listed in curve_types::type_pack, if T is a collection type, string or bool, then CurveType cannot be linear_curve");

			for (auto& entry : g.object_variables)
			{
				if (entry.id == v &&
					entry.info == get_curve_info<CurveType, T>())
						return &(static_cast<state_field<CurveType<T>>*>(entry.var)->data);
			}

			return nullptr;
		}

		template<template<typename> typename CurveType, typename T, typename GameObj>
		inline CurveType<T>& get_object_property_ref(GameObj& g, const variable_id v)
		{
			static_assert(curve_types::is_curve_type_v<T> && (curve_types::is_linear_interpable_v<T> || !std::is_same_v<CurveType<T>, linear_curve<T>>),
				"T must be one of the curve types listed in curve_types::type_pack, if T is a collection type, string or bool, then CurveType cannot be linear_curve");

			for (auto& entry : g.object_variables)
			{
				if (entry.id == v)
				{
					if (entry.info == get_curve_info<CurveType, T>())
						return static_cast<state_field<CurveType<T>>*>(entry.var)->data;
					else
						throw object_property_wrong_type{ "attempted to get property using the wrong type, requested: "
						+ to_string(get_curve_info<CurveType, T>()) + "; stored:"
						+ to_string(entry.info) };
				}
			}
			assert(false);
			throw object_property_not_found{ "object missing expected property " + to_string(v) };
		}
	}

	template<template<typename> typename CurveType, typename T>
	const CurveType<T>& get_object_property_ref(const game_obj& o, variable_id v)
	{
		return detail::get_object_property_ref<const CurveType, T>(o, v);
	}

	template<template<typename> typename CurveType, typename T>
	CurveType<T>& get_object_property_ref(game_obj& o, variable_id v)
	{
		return detail::get_object_property_ref<CurveType, T>(o, v);
	}

	template<template<typename> typename CurveType, typename T>
	CurveType<T>* get_object_property_ptr(game_obj& o, variable_id v) noexcept
	{
		return detail::get_object_property_ptr<T>(o, v);
	}
}
