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
		for (const auto& o : s.objects)
			load(o);
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
