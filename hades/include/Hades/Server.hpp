#ifndef HADES_SERVER_HPP
#define HADES_SERVER_HPP

namespace hades
{
	namespace server
	{
		//the server interface class
		//this is used by gamestates for local/remote analogous requests 
		template<class ClientAction>
		class game_server
		{
		public:
			//user provided type for setting requests or input from the client to the server
			using client_action = ClientAction;

			virtual ~game_server() {}

			//allows this client to send input
			virtual void sendAction(client_action action);
			//gets entity updates
			void getCurveUpdates();
			//returns the unique id of the player entity.
			int getPlayerId();
		};

		//the implementation for a server running in a seperate instance
		template<ClientAction>
		class remote_server : public game_server<ClientAction>
		{};

		//the server that hosts actual game logic and update ticking.
		//this should only be used for command interaction or ticking
		//normal gameplay calls should go through game_server
		template<class ClientAction>
		class local_server : public game_server<ClientAction>
		{};
	}
}

#endif //HADES_SERVER_HPP