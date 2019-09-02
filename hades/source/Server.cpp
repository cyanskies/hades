#include "Hades/Server.hpp"

#include "Hades/game_system.hpp"
#include "hades/game_instance.hpp"

namespace hades
{
	void register_game_server_resources(data::data_manager &d)
	{
		register_objects(d);
		register_game_system_resources(d);
		//TODO: support scripted or embeded systems
		//register_systems_resources();
	}

	class local_server_hub;

	class local_server_level final : public server_level
	{
	public:
		local_server_level(level_save sv, local_server_hub *server) : _game(sv), _server(server)
		{}

		void tick(time_duration dt)
		{
			_game.tick(dt);
			_level_time += dt;
		}

		void send_request(action a) override
		{
			/*yeeaarrrghhh*/
		}

		void send_request(std::vector<action> a) override
		{
			/*yeeaarrrghhh*/
		}

		void get_changes(exported_curves &exp, time_point dt) const override
		{
			_last_update_time = _level_time;
			return _game.get_changes(exp, dt);
		}

		void get_changes(exported_curves& exp) const override
		{
			return get_changes(exp, _last_update_time);
		}

		virtual const common_interface* get_interface() const noexcept
		{
			return _game.get_interface();
		}

	private:
		game_instance _game;
		
		local_server_hub *_server; 
		//entities

		time_point _level_time;
		time_point _instance_time;
		mutable time_point _last_update_time;
	};

	//TODO: handle advertising local network and accepting connections from remote_server_hubs
	class local_server_hub final : public server_hub
	{
	public:
		local_server_hub(level_save lvl) : _level{ lvl, this }//, _game_instance{lvl}
		{}

		void update(time_duration dt) override
		{
			_server_time += dt;
			//tick the mission contruct
			//_game_instance.tick(dt);

			//tick all the level contructs
			//they will give accesss to the mission construct as well.
			_level.tick(dt);
		}

		void get_updates(exported_curves& exp, time_point dt) const override
		{
			_last_local_update_request = _server_time;
			return _level.get_changes(exp, dt);
		}

		void get_updates(exported_curves &exp) const override
		{
			const auto dt = _last_local_update_request;
			return get_updates(exp, dt);
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
		//game_instance _game_instance;
		//players
		
		mutable time_point _last_local_update_request;
		time_point _server_time;
		time_point _start_time;

		//levels
		local_server_level _level;
	};

	//has to handle networking and resulving unique_id differences between hosts
	//remote server hub

	std::unique_ptr<server_hub> create_server(level_save lvl)
	{
		return std::make_unique<local_server_hub>(lvl);
	}
}