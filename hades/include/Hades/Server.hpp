#ifndef HADES_SERVER_HPP
#define HADES_SERVER_HPP

#include "SFML/System/Time.hpp"

#include "Hades/ExportedCurves.hpp"
#include "hades/input.hpp"
#include "Hades/mission.hpp"
#include "Hades/level.hpp"

namespace hades
{
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
		virtual exported_curves get_changes(sf::Time) = 0;
		//sends player input
		virtual void handle_request(hades::action a) = 0;
		//
		virtual exported_curves resync() = 0;

	private:
		//level_id
		unique_id level_id = unique_id::zero;
		//server_hub_ptr
		std::shared_ptr<server_hub> _server_hub;
	};

	//core server hub, this is used when creating the server, or first connecting
	class server_hub
	{
	public:
		virtual void update(sf::Time) = 0; //noop on remote server
		virtual void send_request(/*level_target*//*action*/) = 0;
		virtual exported_curves get_updates(sf::Time) = 0;
		virtual exported_curves resync(sf::Time) = 0;

		//get source_file level1.mission or whatever
		virtual void get_mission() = 0;

		virtual server_level* connect_to_level() = 0;
		virtual void connect_to_level(/*level token*/int) = 0;
		virtual void disconnect_from_level() = 0;
	};

	//starting point
	//std::shared_ptr<server_hub> create_server(mission_save mis);
	std::unique_ptr<server_hub> create_server(level_save lvl);

	std::unique_ptr<server_hub> connect_to_server(/*ip address*/);
}

#endif //HADES_SERVER_HPP