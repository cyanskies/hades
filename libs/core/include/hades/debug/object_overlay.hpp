#ifndef HADES_OBJECT_OVERLAY_HPP
#define HADES_OBJECT_OVERLAY_HPP

#include "SFML/Graphics/Text.hpp"

#include "hades/curve_types.hpp"
#include "hades/debug.hpp"
#include "hades/game_state.hpp"
#include "hades/font.hpp"
#include "hades/level_interface.hpp"
#include "hades/properties.hpp"
#include "hades/uniqueid.hpp"

namespace hades::debug
{
	// displays object information
	class object_overlay : public text_overlay
	{
	public:
		object_overlay(common_interface*, game_interface* = nullptr, object_ref = curve_types::bad_object_ref, bool sticky = false);
		
		object_ref get_obj() const noexcept
		{
			return _object;
		}

		bool sticky_selection() const noexcept
		{
			return _sticky;
		}

		void set_time(time_point t)
		{
			_time = t;
			if(_game)
				_name = state_api::get_name(_object, t, _game->get_state());
		}

		string update() override;

	private:
		struct curve_struct
		{
			string name;
			void* curve;
			using fn = string(*)(std::string_view, void*, time_point);
			fn print;
		};

		string _type, _name; 
		std::vector<curve_struct> _curves;
		object_ref _object;
		common_interface* _level;
		game_interface* _game;
		bool _sticky;
		time_point _time;
	};
}

#endif // !HADES_OBJECT_OVERLAY_HPP
