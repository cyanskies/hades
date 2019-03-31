#ifndef HADES_SERVER_HPP
#define HADES_SERVER_HPP

#include "hades/export_curves.hpp"
#include "hades/input.hpp"
#include "hades/mission.hpp"
#include "hades/level.hpp"
#include "hades/timers.hpp"

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
		//pumps the network and prepares for get_changes and handle_request
		virtual void update() = 0; 
		//gets updated curves
		virtual exported_curves get_changes(time_point) = 0;
		//sends player input
		virtual void send_request(action a) = 0;
		virtual void send_request(std::vector<action>) = 0;
		//
		virtual exported_curves resync() = 0;
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
		virtual exported_curves get_updates(time_point) = 0;
		//get server state at time_point
		virtual exported_curves resync(time_point) = 0;

		//get source_file level1.mission or whatever
		virtual void get_mission() = 0;

		virtual server_level* connect_to_level(unique_id) = 0;
		virtual void disconnect_from_level() = 0;
	};

	using server_ptr = std::unique_ptr<server_hub>;

	//starting point
	//std::shared_ptr<server_hub> create_server(mission_save mis);
	server_ptr create_server(level_save lvl);
	server_ptr connect_to_server(/*ip address*/);
}

#endif //HADES_SERVER_HPP