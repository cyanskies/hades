#ifndef HADES_SERVER_HPP
#define HADES_SERVER_HPP

namespace hades
{
	namespace server
	{
		//the server interface class
		class game_server
		{
		public:
			virtual ~game_server() {}


		};

		//the implementation for a server running in a seperate instance
		class remote_server;

		//the server that hosts actual game logic and update ticking.
		class local_server;
	}
}

#endif //HADES_SERVER_HPP