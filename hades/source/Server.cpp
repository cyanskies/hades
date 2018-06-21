#include "Hades/Server.hpp"

namespace hades
{
	class local_server_level
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
		//game instance
		//server_hub_ref
		//entities
	};

	//
	class local_server_hub : public server_hub
	{
	public:
		void update();
		void send_request();
	private:
		//game instance
		//players
		//mission level objects
		//levels
	};
}