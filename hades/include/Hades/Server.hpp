#ifndef HADES_SERVER_HPP
#define HADES_SERVER_HPP

namespace hades
{
	namespace server
	{
		//this is for messages the game state will have to handle
		class ServerMessage
		{
		public:
			enum MessageType {
				// Connection Management
				DISCONNECTED, RECONNECT_REQUEST, //this client has been disconnected, or is being requested to disconnect and reconnect as the same slot
				// Game Management
				PLAYER_SLOT_CHANGED, //if this client is now in control of a different player slot
				//Game Functions
				CHAT_MESSAGE, TEAM_MESSAGE,
				GAME_OVER, //list of players who won, and the world exit they achieved

			};
		};

		//the server interface class
		//this is used by gamestates for local/remote analogous requests 
		template<class ClientAction, class ServerMessage>
		class game_server
		{
		public:
			//user provided type for setting requests or input from the client to the server
			using client_action = ClientAction;
			using server_message = ServerMessage;

			virtual ~game_server() {}

			//allows this client to send input/commands (Move unit, quit game , etc)
			virtual void sendAction(client_action action) = 0;
			//get messages from the server(non gameplay curves, notifications, like, kicked from game, reconnect request, game chat?)
			virtual server_message getServerUpdates() = 0;
			//gets entity updates
			void getCurveUpdates();
			//returns the unique id of the player entity.
			int getPlayerId();
		};

		//this is for internal messaging that the game_server -> remote_server will handle internally
		class ConnectionMessage {};

		//the implementation for a server running in a seperate instance
		template<class ClientAction, class ServerMessage>
		class remote_server : public game_server<ClientAction, ServerMessage>
		{};

		//the server that hosts actual game logic and update ticking.
		//this should only be used for command interaction or ticking
		//normal gameplay calls should go through game_server
		//wraps GameInstance in the server API
		template<class ClientAction, class ServerMessage>
		class local_server : public game_server<ClientAction, ServerMessage>
		{};
	}
}

#endif //HADES_SERVER_HPP