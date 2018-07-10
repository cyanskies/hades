#include "Hades/Server.hpp"
#include "Hades/GameInstance.hpp"

namespace hades
{
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

		void handle_request() override
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
		local_server_hub(level_save lvl) : _level(lvl)
		{}

		virtual void update(sf::Time dt)
		{
			tick(dt);
		}

		void tick(sf::Time dt)
		{
			_game_instance.tick(dt);
			_level.tick(dt);
		}

		virtual void send_request(/*action*/);
		virtual ExportedCurves get_updates(sf::Time dt)
		{
			return _level.get_changes(dt);
		}

		virtual ExportedCurves resync(sf::Time)
		{
			return get_updates(sf::Time{});
		}

		//get source_file name level1.mission or whatever

		virtual server_level* connect_to_level()
		{
			return &_level;
		}

		//virtual void connect_to_level(/*level token*/int) = 0;
		virtual void disconnect_from_level() = 0;

	private:
		//mission game instance
		GameInstance _game_instance;
		//players
		
		//levels
		local_server_level _level;
	};

	std::shared_ptr<server_hub> create_server(level_save lvl)
	{
		return std::make_shared<local_server_hub>(lvl);
	}
}