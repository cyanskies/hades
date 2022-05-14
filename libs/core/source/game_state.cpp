#include "hades/game_state.hpp"

namespace hades::detail
{
	constexpr auto bad_size = std::numeric_limits<std::size_t>::max();
	static constexpr std::size_t find_first_empty(const std::vector<bool>& e) noexcept
	{
		const auto siz = size(e);
		for (auto i = std::size_t{}; i < siz; ++i)
		{
			if (e[i] == game_object_collection::empty_flag)
				return i;
		}
		return bad_size;
	}

	game_obj* game_object_collection::insert()
	{
		const auto empty_index = find_first_empty(_emptys);
		++_size;
		if (empty_index > std::size(_emptys))
		{
			// this is why we're not noexcept
			_emptys.emplace_back(not_empty_flag);
			return &_data.emplace_back(game_obj{});
		}

		_emptys[empty_index] = not_empty_flag;
		return &_data[empty_index];
	}

	game_obj* game_object_collection::insert(game_obj o)
	{
		auto ptr = insert();
		*ptr = std::move(o);
		return ptr;
	}

	void game_object_collection::erase(game_obj* o) noexcept
	{
		assert(o);
		const auto size = std::size(_emptys);
		for (auto i = std::size_t{}; i < size; ++i)
		{
			if (_data[i].id == o->id)
			{
				_emptys[i] = empty_flag;
				--_size;
				return;
			}
		}

		return;
	}

	template<typename EmptyList, typename DataDeque>
	auto game_obj_find_impl(const entity_id e, EmptyList& emptys, DataDeque& data) 
		-> std::conditional_t<std::is_const_v<EmptyList>, const game_obj*, game_obj*>
	{
		const auto siz = std::size(emptys);
		for (auto i = std::size_t{}; i < siz; ++i)
		{
			auto ptr = &data[i];
			if (ptr->id == e)
			{
				if (emptys[i] == game_object_collection::empty_flag)
					return nullptr;
				else
					return ptr;
			}
		}
		return nullptr;
	}

	const game_obj* game_object_collection::find(const entity_id e) const noexcept
	{
		return game_obj_find_impl(e, _emptys, _data);
	}

	game_obj* game_object_collection::find(const entity_id e) noexcept
	{
		return game_obj_find_impl(e, _emptys, _data);
	}
}

namespace hades::state_api
{
	time_point get_object_creation_time(const game_obj& o, const game_state& s)
	{
		return s.object_creation_time.at(o.id);
	}

	void name_object(string s, object_ref o, time_point t, game_state& g)
	{
		auto& name = g.names[std::move(s)];
		name.add_keyframe(t, o);
		return;
	}

	string get_name(const object_ref o, const time_point t, const game_state& s)
	{
		for (const auto &[name, objects] : s.names)
		{
			if (objects.get(t) == o)
				return name;
		}

		return {};
	}

	bool is_object_stale(object_ref& o) noexcept
	{
		const auto stale = !(o.ptr && o.ptr->id != bad_entity
			&& o.ptr->id == o.id);
		if (stale)
			o.ptr = nullptr;
		return stale;
	}
}
