#ifndef HADES_SERVER_HPP
#define HADES_SERVER_HPP

#include "hades/export_curves.hpp"
#include "hades/input.hpp"
#include "hades/mission.hpp"
#include "hades/level.hpp"
#include "hades/time.hpp"

//TODO: move to hades-core
//	replace get_changes return with a struct containing curves and other data
//  need to tell the client the target gametime they should be drawing

// __levels and time__
// game time is the mission time
// systems updating in all levels need to use the mission time
// otherwise inter-level comms will be broken

// levels need to be independently pausable.
// levels will have to store an offset time, accumulated as long as they were inactive for
// the offset will have to be applied when accessing curves from inside the level
// this will need to be saved to the level

//NOTE: current system doesn't support this, use mission time everywhere

namespace hades
{
	void register_game_server_resources(data::data_manager&);
	//players hold shared ptrs to the area they are connected too

	class server_hub;
	//server level connections
	//client game states, use this to play
	class server_level
	{
	public:
		virtual ~server_level() noexcept = default;

		//gets updated curves
		//recommended to keep the exported_curves object between frames
		//get all changes since time_point
		virtual void get_changes(exported_curves&, time_point) const = 0;
		//get all changes since last call to get_changes
		virtual void get_changes(exported_curves&) const = 0;
		//sends player input
		virtual void send_request(unique_id, std::vector<action>) = 0;

		virtual common_interface* get_interface() noexcept = 0;
		// returns nullptr if this the game_interface is not available
		virtual game_interface* try_get_game_interface() noexcept = 0;

		//current level time
		//virtual time_point get_level_time() = 0;
		//time the level started at, from level save
		//virtual time_point get_level_start_time() = 0;
	};

	using level_ptr = server_level*;

	//core server hub, this is used when creating the server, or first connecting
	//can also we accessed for mission curves
	class server_hub
	{
	public:
		virtual ~server_hub() noexcept = default;

		//advance server by duration
		virtual void update(time_duration) = 0; //noop on remote server
		//virtual void send_request(unique_id lvl, action a) = 0;
		//get all updates since time_point
		//NOTE: these are for getting mission state, not level state
		virtual void get_updates(exported_curves&, time_point) const = 0;
		//get all updates since last call to any get_updates func
		virtual void get_updates(exported_curves&) const = 0;
		//get server state at time_point
		//virtual exported_curves resync(time_point) = 0;

		//get the server start time, either 0,
		//or the time loaded from the save file
		//virtual time_point get_start_time() = 0;
		//get the servers current time
		//virtual time_point get_current_time() = 0;
		//returns the total mission time of the server
		virtual time_point get_time() const noexcept = 0;

		//returns the mission interface
		virtual common_interface* get_interface() noexcept = 0;
		//get the object ref for this player in the mission interface
		// returns bad_entity on failure
		virtual object_ref get_player_obj(unique_id) noexcept = 0;

		//get source_file level1.mission or whatever
		virtual mission get_mission() = 0;

		virtual server_level *connect_to_level(unique_id) = 0;
		virtual void disconnect_from_level() = 0;
	};

	using server_ptr = std::unique_ptr<server_hub>;

	//server exceptions
	//generic server error
	struct server_error : runtime_error
	{
		using runtime_error::runtime_error;
	};

	//mission save structure is invalid(including mission source if so)
	struct bad_mission_save : server_error
	{
		using server_error::server_error;
	};

	//level save structure is invalid. including source if so
	struct bad_level_save : server_error
	{
		using server_error::server_error;
	};

	//starting point
	//these can throw exceptions relating to invalid saves and play slots
	server_ptr create_server(mission_save); //auto player assignment
	server_ptr create_server(mission_save, unique_id player_slot); //join as player x // obs for slot -1 or already taken 
	server_ptr create_server(mission_save, std::string_view name_slot); //join as player in named slot
	//server_ptr connect_to_server(/*ip address*/); //auto player assignment
}

#endif //HADES_SERVER_HPP
