#ifndef HADES_PLAYERS_HPP
#define HADES_PLAYERS_HPP

#include "hades/types.hpp"

// This header defines the player functionality
// that is visible on the server, clients only see 
// other players if the game exposes them.

namespace hades
{
	//NOTE: this limits to 127 players
	//		and 128 observers
	//		0 is invalid
	using player_index = int8;

	//player object for the game instance
	//needs the player id and an any_map

	//observers have no any_map and cannot sent input
}

#endif //!HADES_PLAYERS_HPP