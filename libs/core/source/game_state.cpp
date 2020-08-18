#include "hades/game_state.hpp"

namespace hades::state_api
{
	bool name_object(string s, object_ref o, game_state& g)
	{
		const auto [iter, success] = g.names.try_emplace(std::move(s), o);
		return success;
	}
}
