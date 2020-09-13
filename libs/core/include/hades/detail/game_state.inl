#include "hades/game_state.hpp"

namespace hades::state_api
{
	namespace
	{
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
				auto& colony = std::get<game_state::data_colony<T>>(s.state_data);
				auto data = state_field<T>{ obj.id, c->id, std::move(v) };
				auto iter = colony.emplace(std::move(data));
				assert(iter != end(colony));
				auto& vars = std::get<game_obj::var_list<T>>(obj.object_variables);
				vars.emplace_back(game_obj::var_entry<T>{c->id, &* iter});
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

		auto obj = e.objects.emplace(game_obj{ id, o.obj_type });
		assert(obj != end(e.objects));

		for (auto& [c, v] : get_all_curves(o))
		{
			assert(c);
			assert(!v.valueless_by_exception());
			assert(resources::is_set(v));
			std::visit(make_object_visitor{ c, s, *obj }, v);
		}

		if (!empty(o.name_id))
			name_object(o.name_id, { id, &*obj }, s);

		for (const auto sys : get_systems(*obj->object_type))
			e.systems.attach_system({ id, &*obj }, sys->id);

		return { id, &*obj };
	}

	template<typename GameSystem>
	object_ref get_object_ref(std::string_view s, game_state& g, extra_state<GameSystem>& e) noexcept
	{
		const auto obj = g.names.find(string(s));
		if (obj == end(g.names))
		{
			LOGWARNING("tried to find named object that doesn't exist, name was: " + string{ s });
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
			auto obj_ptr = std::find_if(begin(e.objects), end(e.objects), [id = o.id](auto&& o) {
				return id == o.id;
			});

			//entity is gone, goodbye
			if (obj_ptr == end(e.objects))
			{
				LOGWARNING("stale object ref, the object is no longer with us");
				return nullptr;
			}

			o.ptr = &*obj_ptr;
			return o.ptr;
		}

		if (o.id != o.ptr->id)
		{
			LOGWARNING("stale object ref, the ptr has been resused for a new object");
			return nullptr;
		}

		return o.ptr;
	}

	namespace detail
	{
		template<typename T, typename GameObj>
		T& get_object_property_ref(GameObj& g, const variable_id v)
		{
			auto& var_list = std::get<GameObj::var_list<std::decay_t<T>>>(g.object_variables);
			auto var_iter = std::find_if(begin(var_list), end(var_list),
				[v](GameObj::var_entry<std::decay_t<T>>& elm) {
				return v == elm.id;
				});

			if (var_iter == end(var_list))
				throw object_property_not_found{"object property not found: " + to_string(v)};

			return var_iter->var->data;
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
}
