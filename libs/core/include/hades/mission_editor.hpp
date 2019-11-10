#ifndef HADES_MISSION_EDITOR_HPP
#define HADES_MISSION_EDITOR_HPP

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
			bool dirty = false;
		};

		void init() override;
		bool handle_event(const event&) override;
		void update(time_duration dt, const sf::RenderTarget& t, input_system::action_set a) override;
		void draw(sf::RenderTarget&, time_duration) override;
		void reinit() override;
		void resume() override;

		
	protected:

		// create the editor state for a level
		virtual void make_editor_state(const mission_editor_t*, level*, unique_id) = 0;
		// generate the starting state for a new mission
		virtual mission new_mission();

	private:
		void _gui_menu_bar();
		void _gui_level_window();
		void _gui_object_window();
		void _update_gui(time_duration dt);		

		// ==gui vars==
		bool _level_w = true, _obj_w = true;
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
		void make_editor_state(const mission_editor_t* m, level* l, unique_id id) override
		{
			//static_assert(std::is_constructible_v<LevelEditor, const mission_editor_t*, level*, unique_id>);
			return;
		}
	};

	using mission_editor = basic_mission_editor<level_editor>;
}

#endif //!HADES_MISSION_EDITOR_HPP
