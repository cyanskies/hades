#ifndef HADES_RENDER_INSTANCE_HPP
#define HADES_RENDER_INSTANCE_HPP

#include <unordered_set>

#include "hades/export_curves.hpp"
#include "hades/level_interface.hpp"
#include "hades/level.hpp"
#include "hades/time.hpp"

namespace hades
{
	class render_instance
	{
	public:
		explicit render_instance(common_interface*);

		void create_new_objects(std::vector<game_obj>);
		// TODO: dead object update

		extra_state<render_system>& get_extras() noexcept;
		//TODO: should this sync the time between server client?
		// YES, since our curve data is all still in server time
		void make_frame_at(time_point t, const common_interface *mission, render_interface &output);

	private:
		time_point _current_frame;
		time_point _prev_frame{};

		common_interface* _interface = nullptr;
		extra_state<render_system> _extra;
		std::unordered_set<entity_id> _activated_ents;
		//render_implementation _game;
	};
}

#endif //HADES_GAMEINSTANCE_HPP
