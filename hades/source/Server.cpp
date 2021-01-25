#include "Hades/Server.hpp"

#include "hades/game_system.hpp"
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

	static system_job_data make_game_struct(system_job_data d, game_interface* g, game_interface* m,
		time_duration dt, const std::vector<player_data>* p) noexcept
	{
		d.level_data = g;
		d.mission_data = m;
		d.dt = dt;
		d.players = p;
		return d;
	};

	const std::vector<player_data>* get_players_helper(local_server_hub& s);

	class local_server_level final : public server_level
	{
	public:
		local_server_level(const level_save& sv, local_server_hub *server) : _game(sv), _server(server)
		{}

		void tick(time_duration dt, const std::vector<player_data>* p)
		{
			auto data = system_job_data{ _level_time, &_game.get_extras(), &_game.get_systems() };
			game_implementation* mission = nullptr;
			_level_time = update_level(std::move(data), dt, _game, mission, p, make_game_struct);

			//TODO: perhaps only clean this up when requested
			// when running a listen server, this could mean the client won't have
			// any state to read while running disconnect on these objects
			for (const auto o : _game.get_destroyed_objects())
			{
				assert(o);
				state_api::erase_object(*o, _game.get_state(), _game.get_extras());
			}
		}

		void send_request(unique_id id, std::vector<action> a) override
		{
			// TODO: verify that the id represents this client
			_game.update_input_queue(id, std::move(a), _level_time);
			return;
		}

		//TODO: exported curves is dead
		void get_changes(exported_curves &exp, time_point dt) const override
		{
			return;
		}

		void get_changes(exported_curves& exp) const override
		{
			return;
		}

		common_interface* get_interface() noexcept override
		{
			return &_game;
		}

	private:
		game_implementation _game;
		
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
			for (const auto& p : _mission.source.players)
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
			//_mission_instance->tick(dt, &_players);
			_mission_time += dt;
			//tick all the level contructs
			//they will give accesss to the mission construct as well.
			for(auto &l : _levels)
				l.instance.tick(dt, &_players);
		}

		time_point get_time() const noexcept override
		{
			return _mission_time;
		}

		void get_updates(exported_curves& exp, time_point dt) const override
		{
			//_last_local_update_request = _mission_instance->get_time();
			return;
			//return _level.get_changes(exp, dt);
		}

		void get_updates(exported_curves &exp) const override
		{
			const auto dt = _last_local_update_request;
			return get_updates(exp, dt);
		}

		common_interface* get_interface() noexcept override
		{
			assert(_mission_instance);
			return &*_mission_instance;
		}

		object_ref get_player_obj(unique_id i) noexcept override
		{
			for (const auto& p : _players)
			{
				if (p.name == i)
					return p.player_object;
			}

			return {};
		}

		const std::vector<player_data>* get_players()
		{
			return &_players;
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

		std::optional<game_implementation> _mission_instance;
		time_point _mission_time;
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


	const std::vector<player_data>* get_players_helper(local_server_hub& s)
	{
		return s.get_players();
	}

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
