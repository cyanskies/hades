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
		std::size_t index = bad_player_index;
		unique_id name; // the name for this player slot(eg. enemies, player, neutrals)
		string display_name; // the visible title for this player slot(eg. garm brood)

		resources::curve_types::object_ref player_object;
	};

	//player object for the game instance
	//needs the player id and an any_map

	//observers have no any_map and cannot sent input
}

#endif //!HADES_PLAYERS_HPP