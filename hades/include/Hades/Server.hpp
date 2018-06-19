#ifndef HADES_SERVER_HPP
#define HADES_SERVER_HPP

#include "hades/input.hpp"

namespace hades
{
	//players hold shared ptrs to the area they are connected too

	//core server hub, this is used when creating the server, or first connecting
	class server_hub
	{
	public:
		void update(/*dtime*/) = 0; //noop on remote server
		void send_request(/*level_target*//*action*/) = 0;
		void get_updates(/*level_target*/) = 0;
		void resync(/*level token*/);

		void connect_to_level(/*level token*/) = 0;
		void disconnect_from_level() = 0;
	};

	class server_level
	{
	public:
		//create(level src, level save)

		void update();
		void handle_request();
		void get_changes();
		void resync();

		void add_player();
		void remove_player();
	private:
		//server_hub_ref
		//entities
	};

	class local_server_hub
	{
	public:
		void update();
		void send_request();
	private:
		//players
		//mission level objects
		//levels
	};

	//same as local, except it connects to a remote local_server_hub
	class remote_server_hub;

	//server level connections
	//client game states, use this to play
	class server_level_connection
	{
	public:
		//tick
		//get upadtes
		//send input
		//full sync
	private:
		//level_id
		//server_hub
	};
	
	//starting point
	server_hub create_server(/*mission save*/);
	server_hub connect_to_server(/*ip address*/);
}

#endif //HADES_SERVER_HPP