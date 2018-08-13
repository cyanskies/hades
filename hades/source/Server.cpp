#include "Hades/Server.hpp"
#include "Hades/GameInstance.hpp"

namespace hades
{
	class local_server_hub;

	class local_server_level : public server_level
	{
	public:
		local_server_level(level_save sv, local_server_hub &server) : _game(sv), _server(server)
		{}

		void update() override
		{}

		void tick(sf::Time dt)
		{
			_game.tick(dt);
		}

		void handle_request(hades::action a) override
		{
			/*yeeaarrrghhh*/
		}

		ExportedCurves get_changes(sf::Time dt) override
		{
			return _game.getChanges(dt);
		}

		ExportedCurves resync() override
		{
			return _game.getChanges(sf::Time{});
		}

	private:
		GameInstance _game;
		
		local_server_hub &_server; 
		//entities
	};

	class local_server_hub : public server_hub
	{
	public:
		local_server_hub(level_save lvl) : _level{ lvl, *this }, _game_instance{lvl}
		{}

		void update(sf::Time dt) override
		{
			tick(dt);
		}

		void tick(sf::Time dt)
		{
			_game_instance.tick(dt);
			_level.tick(dt);
		}

		void send_request() override
		{

		}

		ExportedCurves get_updates(sf::Time dt) override
		{
			return _level.get_changes(dt);
		}

		ExportedCurves resync(sf::Time) override
		{
			return get_updates(sf::Time{});
		}

		void get_mission() override
		{

		}

		server_level* connect_to_level() override
		{
			return &_level;
		}

		void connect_to_level(/*level token*/int) override
		{}

		void disconnect_from_level() override
		{}

	private:
		//mission game instance
		GameInstance _game_instance;
		//players
		
		//levels
		local_server_level _level;
	};

	std::unique_ptr<server_hub> create_server(level_save lvl)
	{
		return std::make_unique<local_server_hub>(lvl);
	}
}