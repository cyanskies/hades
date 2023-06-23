#include "hades/game_state.hpp"

#include "hades/tuple.hpp"

namespace hades::state_api
{
	namespace detail
	{
		template<template<typename> typename CurveType, typename T>
		static inline void create_object_property(game_obj& object, const unique_id curve_id,
			game_state& state, std::vector<object_save_instance::saved_curve::saved_keyframe> value)
		{
			static_assert(!std::is_same_v<CurveType<T>, const_curve<T>>);
			//add the curve and value into the game_state database
			//then record a ptr to the data in the game object
			auto& colony = std::get<game_state::data_colony<CurveType, T>>(state.state_data);
			auto curve = CurveType<T>{};

			if constexpr (std::is_same_v<CurveType<T>, const_curve<T>>)
			{
				// TODO: move const curves out of game objects
				//		just ref them from the object resource?
				assert(!std::empty(value));
				curve.set(std::move(std::get<T>(value[0].value)));
			}
			else
			{
				curve.reserve(size(value));
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
			const resources::curve& c;
			game_state& s;
			game_obj& obj;
			std::vector<object_save_instance::saved_curve::saved_keyframe> keyframes;

			template<typename Ty, std::enable_if_t<curve_types::is_linear_interpable_v<std::decay_t<Ty>>, int> = 0>
			void operator()(Ty& v)
			{
				//add the curve and value into the game_state database
				//then record a ptr to the data in the game object
				using T = std::decay_t<decltype(v)>;
                switch (c.frame_style)
				{
				case keyframe_style::const_t:
					return;// "const curve cannot be stored in game state"
				case keyframe_style::linear:
					return create_object_property<linear_curve, T>(obj, c.id, s, std::move(keyframes));
				case keyframe_style::pulse:
					return create_object_property<pulse_curve, T>(obj, c.id, s, std::move(keyframes));
				case keyframe_style::step:
					return create_object_property<step_curve, T>(obj, c.id, s, std::move(keyframes));
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
                switch (c.frame_style)
				{
				case keyframe_style::const_t:
					return;// "const curve cannot be stored in game state"
				case keyframe_style::linear:
					throw hades::logic_error{ "linear curve on wrong type" };
				case keyframe_style::pulse:
					return create_object_property<pulse_curve, T>(obj, c.id, s, std::move(keyframes));
				case keyframe_style::step:
					return create_object_property<step_curve, T>(obj, c.id, s, std::move(keyframes));
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
			// give this instance a new unique id if it doesn't have one
			const auto id = o.id == bad_entity ? increment(s.next_id) : o.id;

			// insert always returns a valid ptr
            auto obj = e.objects.insert(game_obj{ id, o.obj_type, {} });
			assert(obj);

			auto curves = get_all_curves(o);

			for (auto& c : curves)
			{
                assert(c.curve_ptr);
				assert(!c.value.valueless_by_exception());
				assert(resources::is_set(c.value));
                std::visit(detail::make_object_visitor{ *c.curve_ptr, s,
					*obj, std::vector{ object_save_instance::saved_curve::saved_keyframe{t, c.value} } }, c.value);
			}

			// no longer true now that we no longer store const curves
			//assert(size(curves) == size(obj->object_variables));

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
            auto obj = e.objects.insert(game_obj{ id, o.obj_type, {} });
			assert(obj);

			//list of curve ids
			auto ids = std::vector<unique_id>{};
			ids.reserve(size(o.curves));

			//add the saved curves
			for (auto& [c, v] : o.curves)
			{
				assert(c);
				assert(!std::empty(v));

				ids.emplace_back(c->id);
				std::visit(detail::make_object_visitor{ *c, s, *obj, v }, v[0].value);
			}

			// no longer true now that we no longer store const curves
			//assert(size(o.curves) == size(obj->object_variables));

			//add the object curves that weren't saved
			std::sort(begin(ids), end(ids));

			const auto& base_curves = resources::object_functions::get_all_curves(*o.obj_type);
			for (auto& c : base_curves)
			{
				//if the id is in the saved list, then don't restore it
                if (std::binary_search(begin(ids), end(ids), c.curve_ptr->id))
					continue;

				auto keyframes = std::vector<object_save_instance::saved_curve::saved_keyframe>{};
				keyframes.push_back({ time_point{}, std::move(c.value) });
                std::visit(detail::make_object_visitor{*c.curve_ptr, s, *obj, keyframes}, keyframes[0].value);
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

			// TODO: attach_system_from_load should accept a ptr, we have a ptr to the system in sys
			for (const auto& sys : resources::object_functions::get_systems(*o.obj_type))
				e.systems.attach_system_from_load(obj, sys.id());

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

		// TODO: we have a ptr to the system in sys, pass that into the extra state
		for (const auto& sys : resources::object_functions::get_systems(*o.obj_type))
			e.systems.attach_system(obj, sys.id());

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
		constexpr void do_call_with_curve_type(Func&&) noexcept
		{
			assert(false);
			return;
		}

		template<template<typename> typename CurveType, typename T, typename Func,
			typename std::enable_if_t<linear_compat<CurveType, T>, int> = 0>
		void do_call_with_curve_type(Func&& f)
		{
            f.template operator()<CurveType, T>();
			return;
		}

		// call this with the curve keyframe style, and Func will be invoked with
		//	the variable type as the second template param
		// NOTE: part of the impl for call_with_curve_info
		template<template<typename> typename CurveType, typename Func>
		void call_with_curve_type(curve_variable_type curve_info, Func&& f)
		{
			// NOTE: use tuple_index_invoke to get the type for this enum value
			// only do this for the type pack, it's so large that writing out the switch is undesirable
			// this generates the same code as the switch would have
			tuple_index_invoke(curve_types::type_pack_instance, integer_cast<std::size_t>(enum_type(curve_info)), [&](const auto& type) {
				return do_call_with_curve_type<CurveType, std::decay_t<decltype(type)>>(std::forward<Func>(f));
				});
		}

		// Calls Func with the correct curve type as a template parameter
		// <template<typename> typename CurveType> Func
		template<curve_type Type, typename Func>
		void call_with_keyframe_style(keyframe_style style, Func&& f)
		{
			using k = keyframe_style;
			switch (style)
			{
			case k::const_t:
				throw logic_error{ "const curves cannot be stored on objects" };
			case k::linear:
			{
				if constexpr (linear_compat<linear_curve, Type>)
					return f.template operator()<linear_curve>();
				break;
			}
			case k::pulse:
				return f.template operator()<pulse_curve>();
			case k::step:
				return f.template operator()<step_curve>();
			case k::end:
				;
			}
			throw logic_error{ "invalid keyframe style" };
		}

		// Calls Func with the correct types as template parameters
		// <template<typename> typename CurveType, typename Type> Func
		template<typename Func>
		void call_with_curve_info(curve_info_t curve_info, Func&& f)
		{
			using k = keyframe_style;
			switch (curve_info.first)
			{
			case k::const_t:
				throw logic_error{ "const curves cannot be stored on objects" };
			case k::linear:
				return call_with_curve_type<linear_curve>(curve_info.second, std::forward<Func>(f));
			case k::pulse:
				return call_with_curve_type<pulse_curve>(curve_info.second, std::forward<Func>(f));
			case k::step:
				return call_with_curve_type<step_curve>(curve_info.second, std::forward<Func>(f));
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

			// copy_curve
			// copies the param at the current time point to the object
			template<typename T>
			void copy_curve(unique_id, const const_curve<T>&)
			{
				throw logic_error{ "const curve cannot be stored in game state" };
			}

			// func for linear and step curves
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
        auto new_obj = extra.objects.insert(game_obj{ id, obj.object_type, {} });

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
		// TODO: we have a ptr to the system in sys, pass that into the extra state
		for (const auto& sys : resources::object_functions::get_systems(*obj.object_type))
			extra.systems.attach_system(ref, sys.id());

		return ref;
	}

	template<typename GameSystem>
	void destroy_object(object_ref o, time_point t, game_state& s, extra_state<GameSystem>& e)
	{
		e.systems.detach_all(o);
		s.object_destruction_time.insert_or_assign(o.id, t);
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
				static_assert(!std::is_same_v<CurveType<T>, const_curve<T>>);

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
		e.objects.erase(&o);
		return;
	}

	template<typename GameSystem>
	object_ref get_object_ref(std::string_view s, time_point t, game_state& g, extra_state<GameSystem>& e) noexcept
	{
		//find and entry for this name
		auto obj = g.names.find(s);
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

	namespace detail
	{
		template<typename ObjRef, typename Extra>
		std::conditional_t<std::is_const_v<ObjRef> || std::is_const_v<Extra>, const game_obj&, game_obj&>
			get_object_impl(ObjRef& o, Extra& e)
		{
			if (o.ptr == nullptr)
			{
				const auto ptr = e.objects.find(o.id);
				//entity is gone, goodbye
				if (ptr == nullptr)
					throw object_stale_error{ "stale object ref, the object is no longer with us" };

				if constexpr(!std::is_const_v<ObjRef>)
					o.ptr = ptr;
				
				return *ptr;
			}

			if (o.ptr->id == bad_entity)
			{
				if constexpr (!std::is_const_v<ObjRef>)
					o.ptr = nullptr;

				throw object_stale_error{ "dead object" };
			}

			if (o.id != o.ptr->id)
			{
				if constexpr (!std::is_const_v<ObjRef>)
					o.ptr = nullptr;

				throw object_stale_error{ "stale object ref, the ptr has been reused for a new object" };
			}

			return *o.ptr;
		}
	}

	template<typename GameSystem>
	game_obj& get_object(object_ref& o, extra_state<GameSystem>& e)
	{
		return detail::get_object_impl(o, e);
	}

	template<typename GameSystem>
	const game_obj& get_object(const object_ref& o, const extra_state<GameSystem>& e)
	{
		return detail::get_object_impl(o, e);
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
		template<template<typename> typename CurveType, typename T>
		inline get_property_return_t<CurveType, T>* get_object_property_ptr(const game_obj& g, const variable_id v) noexcept
		{
			static_assert(curve_types::is_curve_type_v<T> && (curve_types::is_linear_interpable_v<T> || !std::is_same_v<CurveType<T>, linear_curve<T>>),
				"T must be one of the curve types listed in curve_types::type_pack, if T is a collection type, string or bool, then CurveType cannot be linear_curve");

			if constexpr (std::is_same_v<CurveType<T>, const_curve<T>>)
			{
				const auto base_object = g.object_type;
				assert(base_object);
				if (!resources::object_functions::has_curve(*base_object, v))
					return nullptr;

				const auto& prop = resources::object_functions::get_curve(*base_object, v);
                assert(prop.curve_ptr->frame_style == keyframe_style::const_t);
                assert(resources::is_curve_valid(*prop.curve_ptr, prop.value));
				return std::get_if<T>(&prop.value);
			}
			else
			{
				for (auto& entry : g.object_variables)
				{
					if (entry.id == v &&
						entry.info == get_curve_info<CurveType, T>())
						return &(static_cast<state_field<CurveType<T>>*>(entry.var)->data);
				}

				return nullptr;
			}
		}

		template<template<typename> typename CurveType, typename T>
		inline get_property_return_t<CurveType, T>& get_object_property_ref(const game_obj& g, const variable_id v)
		{
			static_assert(curve_types::is_curve_type_v<T> && (curve_types::is_linear_interpable_v<T> || !std::is_same_v<CurveType<T>, linear_curve<T>>),
				"T must be one of the curve types listed in curve_types::type_pack, if T is a collection type, string or bool, then CurveType cannot be linear_curve");

			if constexpr(std::is_same_v<CurveType<T>, const_curve<T>>)
			{
				const auto base_object = g.object_type;
				assert(base_object);
				const auto& prop = resources::object_functions::get_curve(*base_object, v);
                assert(prop.curve_ptr->frame_style == keyframe_style::const_t);
                assert(resources::is_curve_valid(*prop.curve_ptr, prop.value));
				return std::get<T>(prop.value);
			}
			else
			{
				for (auto& entry : g.object_variables)
				{
					if (entry.id == v)
					{
						if (entry.info == get_curve_info<CurveType, T>())
							return static_cast<state_field<CurveType<T>>*>(entry.var)->data;
						else
							throw object_property_wrong_type{ "attempted to get property using the wrong type, requested: "
							+ to_string(get_curve_info<CurveType, T>()) + "; stored: "
							+ to_string(entry.info) };
					}
				}

				assert(false);
				throw object_property_not_found{ "object missing expected property " + to_string(v) };
			} // !else
		}// !get_object_property_ref
	}

	template<template<typename> typename CurveType, typename T>
	const get_property_return_t<CurveType, T>&
		get_object_property_ref(const game_obj& o, variable_id v)
	{
        return detail::get_object_property_ref<CurveType, T>(o, v);
	}

	template<template<typename> typename CurveType, typename T>
	get_property_return_t<CurveType, T>&
		get_object_property_ref(game_obj& o, variable_id v)
	{
        return detail::get_object_property_ref<CurveType, T>(o, v);
	}

	template<template<typename> typename CurveType, typename T>
	const get_property_return_t<CurveType, T>* 
		get_object_property_ptr(const game_obj& o, variable_id v) noexcept
	{
        return detail::get_object_property_ptr<CurveType, T>(o, v);
	}

	template<template<typename> typename CurveType, typename T>
	get_property_return_t<CurveType, T>*
		get_object_property_ptr(game_obj& o, variable_id v) noexcept
	{
        return detail::get_object_property_ptr<CurveType, T>(o, v);
	}

	template<typename GameSystem>
	const tag_list& get_object_tags(object_ref o, const extra_state<GameSystem>& e)
	{
		const auto& obj = get_object(o, e);
		return get_object_tags(obj);
	}

	template<typename T, typename GameSystem>
	T& get_level_local_ref(const unique_id id, extra_state<GameSystem>& extras)
	{
        auto val = extras.level_locals.template try_get<T>(id);
		if (val) return *val;

		static_assert(std::is_default_constructible_v<T>);
		try
		{
            return extras.level_locals.template set<T>(id, {});
		}
		catch (const any_map_value_wrong_type& e)
		{
			throw level_local_wrong_type{ e.what() };
		}
	}

	template<typename T, typename GameSystem>
	const T& get_level_local_ref(unique_id id, const extra_state<GameSystem>& extra)
	{
		try
		{
            return extra.level_locals.template get_ref<T>(id);
		}
		catch (const any_map_key_null& e)
		{
			throw level_local_not_found{ e.what() };
		}
		catch (const any_map_value_wrong_type& e)
		{
			throw level_local_wrong_type{ e.what() };
		}
	}

	template<typename T, typename GameSystem>
	void set_level_local_value(const unique_id id, T value, extra_state<GameSystem>& extras)
	{
		extras.level_locals.set(any_map<unique_id>::allow_overwrite, id, std::move(value));
		return;
	}
}
