#ifndef HADES_MISSION_EDITOR_HPP
#define HADES_MISSION_EDITOR_HPP

#include <optional>
#include <vector>

#include "hades/level.hpp"
#include "hades/level_editor.hpp"
#include "hades/state.hpp"

// The mission editor can be used to create playable missions
// A mission is comprised of multiple levels

namespace hades
{
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
		void update(time_duration dt, const sf::RenderTarget& t, input_system::action_set a) override;
		void reinit() override;
		void resume() override;

		virtual void make_editor_state(const mission_editor_t*, level*, unique_id) = 0;

	private:
		string _mission_name;
		string _mission_desc;

		// players

		//mission level
		std::vector<level_info> _levels;
	};

	//NOTE: LevelEditor must be a hades::state with a constructor that accepts
	//		a const mission*, level* and unique_id of level name
	template<typename LevelEditor>
	class basic_mission_editor : public mission_editor_t
	{
	public:
		using editor_type = LevelEditor;

		void make_editor_state(level_info*, unique_id) override
		{
			//state::push_state(nullptr);
		}
	};

	using mission_editor = basic_mission_editor<level_editor>;
}

#endif //!HADES_MISSION_EDITOR_HPP
