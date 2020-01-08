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

		void send_request(std::vector<server_action> a) override
		{
			_game.add_input(std::move(a), _level_time);
			return;
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
		local_server_hub(mission_save lvl)
		{
			//init the _mission_instance

			for (auto& l : lvl.level_saves)
				_levels.emplace_back(level{ l.name, local_server_level{ std::move(l.save), this } });
		}

		void update(time_duration dt) override
		{
			_server_time += dt;
			//tick the mission contruct
			_mission_instance->tick(dt);

			//tick all the level contructs
			//they will give accesss to the mission construct as well.
			for(auto &l : _levels)
				l.instance.tick(dt);
		}

		time_point get_time() const noexcept override
		{
			return _mission_instance->get_time(_mission_instance->get_time());
		}

		void get_updates(exported_curves& exp, time_point dt) const override
		{
			_last_local_update_request = _server_time;
			return;
			//return _level.get_changes(exp, dt);
		}

		void get_updates(exported_curves &exp) const override
		{
			const auto dt = _last_local_update_request;
			return get_updates(exp, dt);
		}

		void get_mission() override
		{

		}

		server_level* connect_to_level(unique_id id) override
		{
			for (auto& l : _levels)
			{
				if (l.id == id)
					return &l.instance;
			}

			return nullptr;
		}

		void disconnect_from_level() override
		{}

	private:
		mutable time_point _last_local_update_request;
		[[deprecated("get the time from the mission_instance")]]
		time_point _server_time;
		[[deprecated("get the time from the mission_instance")]]
		time_point _start_time;

		std::optional<game_instance> _mission_instance;
		//players

		//levels
		struct level
		{
			unique_id id = unique_id::zero;
			local_server_level instance;
		};

		//NOTE: deque, need constant addresses
		std::deque<level> _levels;
	};

	//has to handle networking and resulving unique_id differences between hosts
	//remote server hub

	std::unique_ptr<server_hub> create_server(mission_save lvl)
	{
		return std::make_unique<local_server_hub>(lvl);
	}
}
