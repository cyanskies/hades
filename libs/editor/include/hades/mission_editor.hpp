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

namespace hades::cvars
{
	constexpr auto editor_mission_ext = "editor_mission_ext";
	constexpr auto editor_lock_player_object_type = "editor_lock_player_object_type";

	namespace default_value
	{
		constexpr auto editor_mission_ext = ".m";
		constexpr auto editor_lock_player_object_type = false;
	}
}

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
            level level_data;
			std::optional<string> path;
		};

		mission_editor_t() = default;
		mission_editor_t(std::filesystem::path p) : _mission_src{ std::move(p) } {}

		void init() override;
		bool handle_event(const event&) override;
		void update(time_duration dt, const sf::RenderTarget& t, input_system::action_set a) override;
		void draw(sf::RenderTarget&, time_duration) override;
		void reinit() override;
		void resume() override;

		using get_players_return_type = detail::level_editor_impl::get_players_return_type;
		get_players_return_type get_players() const;
		
	protected:
		// create the editor state for a level
		virtual void make_editor_state(level_info &l) = 0;
		virtual level make_new_level() = 0;
		// generate the starting state for a new mission
		virtual mission new_mission();
		virtual unique_id default_new_player_object() const noexcept; // TODO: make this into a console property?

	private:
		struct players_window_state_t
		{
			bool open = true;
			std::size_t selected{};
			bool edit_open = false;
		};

		struct level_window_state_t
		{
			enum class errors {
				name_empty,
				name_taken,
				path_not_found,
				end
			};

			bool open = true;
			bool add_level_open = false;
			bool add_level_external = {}; //indicates that the level is stored in a seperate file.
			errors current_error = {};
			bool rename = {};
			string new_level_name;
			string new_level_path;

			int64 rename_index = {};
			std::size_t selected = {};
		};

		struct player_dialog_t
		{
			enum class error_type{
				none,
				name_used,
				name_missing,
				null_entity,
				entity_wrong_type
			};

			bool open = false;
			string name; //the internal slot name
			bool create_ent = true;
			string player_ent;
			error_type error = error_type::none;
		};

		struct save_load_dialog_t
		{
			bool open = false;
			string path;
		};

		using player = mission::player;

		void _gui_menu_bar();
		void _gui_players_window();
		void _gui_level_window();
		void _gui_add_level_window();
		void _gui_object_window();
		void _update_gui(time_duration dt);	
		using path = std::filesystem::path;
		void _save(path);
		void _load(path);

		// ==cvars==
		console::property_str _mission_ext = 
			console::get_string(cvars::editor_mission_ext,
				cvars::default_value::editor_mission_ext);
		console::property_bool _player_lock_object_type =
			console::get_bool(cvars::editor_lock_player_object_type,
				cvars::default_value::editor_lock_player_object_type);

		// ==gui vars==
		bool _obj_w = true;
		bool _obj_prop_w = true;
		level_window_state_t _level_window_state;
		players_window_state_t _player_window_state;
		player_dialog_t _player_dialog;
		save_load_dialog_t _save_window;
		save_load_dialog_t _load_window;
		gui _gui = { "mission_editor" };
		time_point _editor_time;
		vector_int _window_size{};
		sf::View _gui_view;
		sf::RectangleShape _backdrop;

		// ==mission vars==
		path _mission_src;
		string _mission_name;
		string _mission_desc;

		// players
		std::vector<player> _players;

		//levels
		std::vector<level_info> _levels;

		//objects
		using obj_ui = object_editor_ui<object_instance>;
		obj_ui::object_data _objects;
		obj_ui _obj_ui{ &_objects };
	};

	//NOTE: LevelEditor must be a hades::state with a constructor that accepts
	//		a const mission*, level* and unique_id of level name
	template<typename LevelEditor>
	class basic_mission_editor : public mission_editor_t
	{
	public:
		using editor_type = LevelEditor;

		using mission_editor_t::mission_editor_t;

	protected:
		void make_editor_state(level_info &l) override
		{
			static_assert(std::is_constructible_v<LevelEditor, const mission_editor_t*, level*>);
            push_state(std::make_unique<LevelEditor>(this, &l.level_data));
			return;
		}

		// NOTE: this will deactivate gui context
		level make_new_level() override
		{
			static_assert(std::is_constructible_v<LevelEditor>);

			auto editor = LevelEditor{};
			editor.init();
			return editor.get_level();
		}
	};

	using mission_editor = basic_mission_editor<level_editor>;
}

#endif //!HADES_MISSION_EDITOR_HPP
