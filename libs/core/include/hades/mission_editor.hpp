#ifndef HADES_MISSION_EDITOR_HPP
#define HADES_MISSION_EDITOR_HPP

#include <filesystem>
#include <optional>
#include <vector>

#include "hades/gui.hpp"
#include "hades/level.hpp"
#include "hades/level_editor.hpp"
#include "hades/level_editor_objects.hpp"
#include "hades/mission.hpp"
#include "hades/state.hpp"
#include "hades/time.hpp"

// The mission editor can be used to create playable missions
// A mission is comprised of multiple levels

namespace hades
{
	void create_mission_editor_console_variables();
	void register_mission_editor_resources(data::data_manager&);

	class mission_editor_t : public state
	{
	public:
		struct level_info
		{
			unique_id name = unique_id::zero;
			level level;
			std::optional<string> path;
		};

		void init() override;
		bool handle_event(const event&) override;
		void update(time_duration dt, const sf::RenderTarget& t, input_system::action_set a) override;
		void draw(sf::RenderTarget&, time_duration) override;
		void reinit() override;
		void resume() override;
		
	protected:
		// create the editor state for a level
		virtual void make_editor_state(level_info &l) = 0;
		// generate the starting state for a new mission
		virtual mission new_mission();

	private:
		struct players_window_state_t
		{
			bool open = true;
		};

		struct level_window_state_t
		{
			enum class errors {
				name_empty,
				name_taken,
				path_not_found
			};

			bool open = true;
			bool add_level_open = false;
			bool add_level_external; //indicates that the level is stored in a seperate file.
			errors current_error;
			bool rename;
			string new_level_name;
			string new_level_path;

			int64 rename_index;
			std::size_t selected{};
		};

		struct save_load_dialog_t
		{
			bool open = false;
			string path;
		};

		void _gui_menu_bar();
		void _gui_players_window();
		void _gui_level_window();
		void _gui_add_level_window();
		void _gui_object_window();
		void _update_gui(time_duration dt);	
		using path = std::filesystem::path;
		void _save(path);
		void _load(path);

		// ==gui vars==
		bool _obj_w = true;
		level_window_state_t _level_window_state;
		players_window_state_t _player_window_state;
		save_load_dialog_t _save_window;
		save_load_dialog_t _load_window;
		gui _gui;
		time_point _editor_time;
		vector_int _window_size;
		sf::View _gui_view;
		sf::RectangleShape _backdrop;

		// ==mission vars==
		string _mission_src;
		string _mission_name;
		string _mission_desc;

		// players

		//mission level
		std::vector<level_info> _levels;
	};

	//NOTE: LevelEditor must be a hades::state with a constructor that accepts
	//		a const mission*, level* and unique_id of level name
	template<typename LevelEditor>
	class basic_mission_editor final : public mission_editor_t
	{
	public:
		using editor_type = LevelEditor;

	protected:
		void make_editor_state(level_info &l) override
		{
			static_assert(std::is_constructible_v<LevelEditor, const mission_editor_t*, level*>);
			push_state(std::make_unique<LevelEditor>(this, &l.level));
			return;
		}
	};

	using mission_editor = basic_mission_editor<level_editor>;
}

#endif //!HADES_MISSION_EDITOR_HPP
