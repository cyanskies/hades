#ifndef HADES_PLAYERS_HPP
#define HADES_PLAYERS_HPP

#include <cstdint>

#include "hades/types.hpp"
#include "hades/uniqueid.hpp"

#include "hades/game_system.hpp"

// This header defines the player functionality
// that is visible on the server, clients only see 
// other players if the game exposes them.

namespace hades
{
	constexpr auto bad_player_index = std::numeric_limits<std::size_t>::max();

	struct player_data
	{
		enum class state : int8
		{
			begin,
			empty = begin, // can be joined
			local, // same process player
			remote, // remote device player
			closed, // empty, cannot be joined
			end
		};

		unique_id name = unique_zero; // the name for this player slot(eg. enemies, player, neutrals)
		resources::curve_types::object_ref player_object = curve_types::bad_object_ref;
		state player_state = state::end;
	};
}

#endif //!HADES_PLAYERS_HPP
