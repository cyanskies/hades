#ifndef HADES_LEVEL_SCRIPTS_HPP
#define HADES_LEVEL_SCRIPTS_HPP

#include "hades/input.hpp"
#include "hades/resource_base.hpp"
#include "hades/uniqueid.hpp"

namespace hades
{
	struct player_data;
}

namespace hades::data
{
	class data_manager;
}

namespace hades::resources
{
	//player input scripts
	//each level should have a player-input script assigned
	//to convert recieved actions into responses in the game world
	struct player_input_t {};
	struct player_input : resource_type<player_input_t>
	{
		//TODO: this will have to become a std::function when we support input scripts
		using player_input_fn = void(*)(const std::vector<player_data>&, std::vector<action>); // TODO: pass player id here for multi player games

		// either a ptr to the actual function
		// or a lambda that calls an actual script
		player_input_fn function = nullptr;
	};

	void make_player_input_resource(std::string_view name, player_input::player_input_fn, data::data_manager&);
	const std::vector<unique_id>& get_player_input_list() noexcept;

	//ai input(allows ai to perform input into game)
	//each level should have an ai input script to 
	//other level scripts

	//save/load overrides

	//on_tick override
}

namespace hades
{
	void register_level_scripts_resource(data::data_manager&);
}

#endif //HADES_LEVEL_SCRIPTS_HPP
