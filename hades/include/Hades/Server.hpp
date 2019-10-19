#ifndef HADES_SERVER_HPP
#define HADES_SERVER_HPP

#include "hades/export_curves.hpp"
#include "hades/input.hpp"
#include "hades/mission.hpp"
#include "hades/level.hpp"
#include "hades/level_curve_data.hpp"
#include "hades/server_actions.hpp"
#include "hades/timers.hpp"

//TODO: move to hades-core
//	replace get_changes return with a struct containing curves and other data
//  need to tell the client the target gametime they should be drawing

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
		//gets updated curves
		//recommended to keep the exported_curves object between frames
		//get all changes since time_point
		virtual void get_changes(exported_curves&, time_point) const = 0;
		//get all changes since last call to get_changes
		virtual void get_changes(exported_curves&) const = 0;
		//sends player input
		virtual void send_request(std::vector<server_action>) = 0;

		virtual const common_interface* get_interface() const noexcept = 0;

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

		//get source_file level1.mission or whatever
		virtual void get_mission() = 0;

		virtual server_level *connect_to_level(unique_id) = 0;
		virtual void disconnect_from_level() = 0;
	};

	using server_ptr = std::unique_ptr<server_hub>;

	//starting point
	//std::shared_ptr<server_hub> create_server(mission_save mis);
	server_ptr create_server(level_save lvl);
	server_ptr connect_to_server(/*ip address*/);
}

#endif //HADES_SERVER_HPP