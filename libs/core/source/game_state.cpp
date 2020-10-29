#include "hades/game_state.hpp"

namespace hades::detail
{
	constexpr auto bad_size = std::numeric_limits<std::size_t>::max();
	static constexpr std::size_t find_first_empty(const std::vector<bool>& e) noexcept
	{
		const auto siz = size(e);
		for (auto i = std::size_t{}; i < siz; ++i)
		{
			if (e[i] == game_object_collection::empty)
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
			_emptys.emplace_back(not_empty);
			return &_data.emplace_back(game_obj{});
		}

		_emptys[empty_index] = not_empty;
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
		const auto size = std::size(_emptys);
		for (auto i = std::size_t{}; i < size; ++i)
		{
			if (&_data[i] == o)
			{
				_emptys[i] = empty;
				--_size;
				return;
			}
		}

		return;
	}

	game_obj* game_object_collection::find(entity_id e) noexcept
	{
		const auto siz = std::size(_emptys);
		for (auto i = std::size_t{}; i < siz; ++i)
		{
			auto ptr = &_data[i];
			if (ptr->id == e)
			{
				if (_emptys[i] == empty)
					return nullptr;
				else
					return ptr;
			}
		}
		return nullptr;
	}
}

namespace hades::state_api
{
	bool name_object(string s, object_ref o, game_state& g)
	{
		const auto [iter, success] = g.names.try_emplace(std::move(s), o);
		return success;
	}
}
