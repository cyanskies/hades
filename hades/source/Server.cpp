#include "Hades/Server.hpp"

namespace hades
{
	class local_server_level : public server_level
	{
	public:
		void create(level save);

		void update();
		void handle_request();
		void get_changes();
		void resync();

		void add_player();
		void remove_player();
	private:
		//game instance
		//server_hub_ref
		//entities
	};

	class local_server_hub : public server_hub
	{
	public:
		virtual void update(/*dtime*/); //noop on remote server
		virtual void send_request(/*level_target*//*action*/);
		virtual void get_updates(/*level_target*/);
		virtual void resync(/*level token*/);

		//get source_file name level1.mission or whatever

		virtual void connect_to_level() = 0;
		virtual void connect_to_level(/*level token*/int) = 0;
		virtual void disconnect_from_level() = 0;

	private:
		//game instance
		//players
		//mission level objects
		//levels
	};

	std::shared_ptr<server_hub> create_server(level_save lvl)
	{
		std::shared_ptr<server_hub> output;

		//copy curve data into the level


	}
}