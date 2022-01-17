#include "hades/save_load_api.hpp"

#include "hades/game_api.hpp"
#include "hades/game_state.hpp"

namespace hades::game::level
{
	void load(const level_save& s)
	{
		load(s.objects);
		return;
	}

	void load(const object_save_data& s)
	{
		const auto current_time = game::get_last_time();
		for (const auto& o : s.objects)
		{
			// don't load objects that have been destroyed
			if(o.destruction_time > current_time)
				load(o);
		}
		return;
	}

	void load(const object_save_instance& o)
	{
		auto ptr = detail::get_game_level_ptr();
		auto& state = ptr->get_state();
		auto& extra = ptr->get_extras();
		state_api::loading::restore_object(o, state, extra);
		return;
	}
}
