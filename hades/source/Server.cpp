#include "Hades/Server.hpp"

#include "hades/game_system.hpp"
#include "hades/game_instance.hpp"
#include "hades/level.hpp"
#include "hades/players.hpp"

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
		local_server_level(const level_save& sv, local_server_hub *server) : _game(sv), _server(server)
		{}

		void tick(time_duration dt, const std::vector<player_data>* p)
		{
			_game.tick(dt, p);
			_level_time += dt;
		}

		void send_request(unique_id id, std::vector<action> a) override
		{
			// TODO: verify that the id represents this client
			_game.add_input(id, std::move(a), _level_time);
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

		common_interface* get_interface() noexcept override
		{
			return _game.get_interface();
		}

	private:
		game_instance _game;
		
		local_server_hub *_server; 

		time_point _level_time;
		time_point _instance_time;
		mutable time_point _last_update_time;
	};

	//TODO: handle advertising local network and accepting connections from remote_server_hubs
	class local_server_hub final : public server_hub
	{
	public:
		local_server_hub(mission_save lvl, unique_id slot = unique_zero)
			: _mission{ std::move(lvl) }
		{
			assert(slot != unique_zero);
			if (slot == unique_zero) //TODO: needed for lobbies and so on.
				throw server_error{ "auto slots not supported" };

			//copy the player table
			for (auto p : _mission.source.players)
			{
				_players.emplace_back(player_data{ p.id, p.object });
				if (p.id == slot)
				{
					_local_player = p.id;
					_players.back().player_state = player_data::state::local;
				}
			}

			if (_local_player == unique_zero) // failed to find the desired player
				throw server_error{ "no player slot" };

			//TODO: init the _mission_instance
			//	on_load and so on.

			for (auto& l : _mission.level_saves)
				_levels.emplace_back(level{ l.name, local_server_level{ l.save, this } });
		}

		void update(time_duration dt) override
		{
			//tick the mission contruct
			_mission_instance->tick(dt, &_players);

			//tick all the level contructs
			//they will give accesss to the mission construct as well.
			for(auto &l : _levels)
				l.instance.tick(dt, &_players);
		}

		time_point get_time() const noexcept override
		{
			return _mission_instance->get_time(_mission_instance->get_time());
		}

		void get_updates(exported_curves& exp, time_point dt) const override
		{
			_last_local_update_request = _mission_instance->get_time();
			return;
			//return _level.get_changes(exp, dt);
		}

		void get_updates(exported_curves &exp) const override
		{
			const auto dt = _last_local_update_request;
			return get_updates(exp, dt);
		}

		mission get_mission() override
		{
			return _mission.source;
		}

		server_level* connect_to_level(unique_id id) override
		{
			//check that the player is correctly joined before allowing

			//level is running
			for (auto& l : _levels)
			{
				if (l.id == id)
					return &l.instance;
			}

			//load level
			//for (auto& l : _mission.level_saves)
			//{
			//	// TODO: load
			//}

			//for (auto& l : _mission.source.external_levels)
			//{
			//	if (l.name == id)
			//	{
			//		// TODO: how do we know if the file is in a mod or not?
			//		// maybe forget externals
			//		//const auto level_source = files::l.path;
			//	}
			//}

			//create level
			for (auto& l : _mission.source.inline_levels)
			{
				if (l.name == id)
				{
					const auto save = make_save_from_level(l.level);
					auto new_level = level{ id, {save, this} };
					auto &out = _levels.emplace_back(std::move(new_level));
					return &out.instance;
				}
			}

			return nullptr;
		}

		void disconnect_from_level() override
		{}

	private:
		mutable time_point _last_local_update_request;
		//save file for the current game
		//also stores the state for unloaded levels
		mission_save _mission;

		std::optional<game_instance> _mission_instance;
		//players
		//TODO: wrap player_data and add network info etc.
		std::vector<player_data> _players;
		unique_id _local_player = unique_zero;

		//levels
		struct level
		{
			unique_id id = unique_id::zero;
			local_server_level instance;
		};

		//NOTE: deque, need constant addresses
		std::deque<level> _levels;
	};

	//has to handle networking and resolving unique_id differences between hosts
	//remote server hub

	std::unique_ptr<server_hub> create_server(mission_save lvl)
	{
		return std::make_unique<local_server_hub>(std::move(lvl));
	}

	server_ptr create_server(mission_save s, unique_id player_slot)
	{
		return std::make_unique<local_server_hub>(std::move(s), player_slot);
	}

	server_ptr create_server(mission_save s, std::string_view name_slot)
	{
		const auto name = data::get_uid(name_slot);
		return create_server(std::move(s), name);
	}

}
