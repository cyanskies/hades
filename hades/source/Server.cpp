#include "Hades/Server.hpp"

#include "Hades/game_system.hpp"
#include "hades/game_instance.hpp"

namespace hades
{
	void register_game_server_resources(data::data_manager &d)
	{
		register_objects(d);

		//TODO: support scripted or embeded systems
		//register_systems_resources();
	}

	class local_server_hub;

	class local_server_level : public server_level
	{
	public:
		local_server_level(level_save sv, local_server_hub *server) : _game(sv), _server(server)
		{}

		void update() override
		{}

		void tick(time_duration dt)
		{
			_game.tick(dt);
		}

		void send_request(action a) override
		{
			/*yeeaarrrghhh*/
		}

		void send_request(std::vector<action> a) override
		{
			/*yeeaarrrghhh*/
		}

		exported_curves get_changes(time_point dt) override
		{
			return _game.getChanges(dt);
		}

		exported_curves resync() override
		{
			return _game.getChanges(time_point{});
		}

	private:
		GameInstance _game;
		
		local_server_hub *_server; 
		//entities
	};

	class local_server_hub : public server_hub
	{
	public:
		local_server_hub(level_save lvl) : _level{ lvl, this }//, _game_instance{lvl}
		{}

		void update(time_duration dt) override
		{
			tick(dt);
		}

		void tick(time_duration dt)
		{
			//tick the mission contruct
			//_game_instance.tick(dt);

			//tick all the level contructs
			//they will give accesss to the mission construct as well.
			_level.tick(dt);
		}

		/*void send_request() override
		{

		}*/

		exported_curves get_updates(time_point dt) override
		{
			return _level.get_changes(dt);
		}

		exported_curves resync(time_point) override
		{
			return get_updates(time_point{});
		}

		void get_mission() override
		{

		}

		server_level* connect_to_level(unique_id) override
		{
			return &_level;
		}

		void disconnect_from_level() override
		{}

	private:
		//mission game instance
		//GameInstance _game_instance;
		//players
		
		//levels
		local_server_level _level;
	};

	std::unique_ptr<server_hub> create_server(level_save lvl)
	{
		return std::make_unique<local_server_hub>(lvl);
	}
}