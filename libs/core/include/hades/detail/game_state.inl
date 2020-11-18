#include "hades/game_state.hpp"

#include "hades/for_each_tuple.hpp"

namespace hades::state_api
{
	namespace detail
	{
		template<typename T>
		static inline void create_object_property(game_obj& object, unique_id curve_id, game_state& state, T value)
		{
			//add the curve and value into the game_state database
			//then record a ptr to the data in the game object
			auto& colony = std::get<game_state::data_colony<T>>(state.state_data);
			auto data = state_field<T>{ object.id, curve_id, std::move(value) };
			auto iter = colony.emplace(std::move(data));
			assert(iter != end(colony));
			auto& vars = std::get<game_obj::var_list<T>>(object.object_variables);
			vars.emplace_back(game_obj::var_entry<T>{curve_id, &* iter});
			return;
		}

		struct make_object_visitor
		{
			const resources::curve* c;
			game_state& s;
			game_obj& obj;

			template<typename Ty>
			void operator()(Ty& v)
			{
				//add the curve and value into the game_state database
				//then record a ptr to the data in the game object
				using T = std::decay_t<decltype(v)>;
				create_object_property(obj, c->id, s, std::move(v));
				return;
			}

			template<> // uncalled function to keep std::variant happy
			constexpr void operator()<std::monostate>(std::monostate&) { assert(false); return; }
		};
	}

	template<typename GameSystem>
	inline object_ref make_object(const object_instance& o, game_state& s, extra_state<GameSystem>& e)
	{
		assert(o.obj_type);

		// give this instance a new unique id if  it doesn't have one
		const auto id = o.id == bad_entity ? increment(s.next_id) : o.id;

		// insert always returns a valid ptr
		auto obj = e.objects.insert(game_obj{ id, o.obj_type });

		for (auto& [c, v] : get_all_curves(o))
		{
			assert(c);
			assert(!v.valueless_by_exception());
			assert(resources::is_set(v));
			std::visit(detail::make_object_visitor{ c, s, *obj }, v);
		}

		if (!empty(o.name_id))
			name_object(o.name_id, { id, obj }, s);

		for (const auto sys : get_systems(*obj->object_type))
			e.systems.attach_system({ id, obj }, sys->id);

		return { id, obj };
	}

	template<typename GameSystem>
	inline object_ref clone_object(const game_obj& obj, game_state& state, extra_state<GameSystem>& extra)
	{
		const auto id = increment(state.next_id);
		auto new_obj = extra.objects.insert(game_obj{ id, obj.object_type });

		//copy all object properties
		for_each_tuple(obj.object_variables, [&state, &obj = *new_obj](auto&& var_list) {
				// var list should be a vector<game_obj::var_entry<T>>
				for (const auto& entry : var_list)
				{
					assert(entry.var);
					using T = std::decay_t<decltype(entry.var->data)>;
					static_assert(std::is_same_v<std::decay_t<decltype(var_list)>, std::vector<game_obj::var_entry<T>>>);
					detail::create_object_property(obj, entry.id, state, entry.var->data);
				}
				return;
			});

		const auto ref = object_ref{ id, &*new_obj };

		//attach all systems
		for (const auto sys : get_systems(*obj.object_type))
			extra.systems.attach_system(ref, sys->id);

		return ref;
	}

	template<typename GameSystem>
	void detach_object_systems(object_ref o, extra_state<GameSystem>& e)
	{
		e.systems.detach_all(o);
		return;
	}

	template<typename GameSystem>
	void erase_object(game_obj& o, game_state& s, extra_state<GameSystem>& e)
	{
		//erase all data
		for_each_tuple(o.object_variables, [&o, &s](auto&& vars){
			// vars == vector<var_entry> -> value_type == var_entry, T == var_entry::value_type
			// T == curve_type::T
			using T = typename std::decay_t<decltype(vars)>::value_type::value_type;
			if (empty(vars))
				return;
			const auto vars_size = size(vars);

			auto& list = std::get<game_state::data_colony<T>>(s.state_data);
			auto found = std::size_t{};
			auto iter = begin(list);
			while (iter != end(list) && found < vars_size)
			{
				if (iter->object == o.id)
				{
					iter = list.erase(iter);
					++found;
				}
				else
					++iter;
			}
			vars.clear();
		});

		o.id = bad_entity;
		e.objects.erase(&o);
		return;
	}

	template<typename GameSystem>
	object_ref get_object_ref(std::string_view s, game_state& g, extra_state<GameSystem>& e) noexcept
	{
		const auto obj = g.names.find(to_string(s));
		if (obj == end(g.names))
		{
			LOGWARNING("tried to find named object that doesn't exist, name was: " + to_string(s));
			return object_ref{};
		}

		assert(obj->second.id != bad_entity);

		auto ptr = get_object(obj->second, e);
		if (ptr == nullptr)
		{
			g.names.erase(obj);
			return object_ref{};
		}

		obj->second.ptr = ptr;
		return obj->second;
	}

	template<typename GameSystem>
	game_obj* get_object(object_ref& o, extra_state<GameSystem>& e) noexcept
	{
		if (o.ptr == nullptr)
		{
			auto obj_ptr = e.objects.find(o.id);

			//entity is gone, goodbye
			if (obj_ptr == nullptr)
			{
				LOGWARNING("stale object ref, the object is no longer with us");
				return nullptr;
			}

			o.ptr = &*obj_ptr;
			return o.ptr;
		}

		if (o.ptr->id == bad_entity)
		{
			o.ptr = nullptr;
			LOGWARNING("dead object");
			return nullptr;
		}

		if (o.id != o.ptr->id)
		{
			o.ptr = nullptr;
			LOGWARNING("stale object ref, the ptr has been resused for a new object");
		}

		return o.ptr;
	}

	template<typename GameSystem>
	game_obj* get_object_ptr(object_ref& e, extra_state<GameSystem>& o) noexcept
	{
		const auto good_obj = e.ptr && e.ptr->id == e.id;
		if (good_obj)
			return e.ptr;

		auto obj_ptr = e.objects.find(o.id);
		if (obj_ptr == nullptr)
			return nullptr;

		e.ptr = obj_ptr;
		return obj_ptr;
	}

	namespace detail
	{
		template<typename T, typename GameObj>
		T* get_object_property_ptr(GameObj& g, const variable_id v) noexcept
		{
			using list_type = typename GameObj::template var_list<std::decay_t<T>>;
			using entry_type = typename GameObj::template var_entry<std::decay_t<T>>;
			auto& var_list = std::get<list_type>(g.object_variables);
			auto var_iter = std::find_if(begin(var_list), end(var_list),
				[v](const entry_type& elm) {
					return v == elm.id;
				});

			if (var_iter == end(var_list))
				return nullptr;

			return &var_iter->var->data;
		}

		template<typename T, typename GameObj>
		T& get_object_property_ref(GameObj& g, const variable_id v)
		{
			auto out_ptr = get_object_property_ptr<T>(g, v);
			if (!out_ptr)
				throw object_property_not_found{ "object missing expected property " + to_string(v) };

			return *out_ptr;
		}
	}

	template<typename T>
	const T& get_object_property_ref(const game_obj& o, variable_id v)
	{
		return detail::get_object_property_ref<const T>(o, v);
	}

	template<typename T>
	T& get_object_property_ref(game_obj& o, variable_id v)
	{
		return detail::get_object_property_ref<T>(o, v);
	}

	template<typename T>
	T* get_object_property_ptr(game_obj& o, variable_id v) noexcept
	{
		return detail::get_object_property_ptr<T>(o, v);
	}
}
