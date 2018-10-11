#include "hades/core_resources.hpp"

#include "hades/animation.hpp"
#include "hades/curve_extra.hpp"
#include "hades/objects.hpp"
#include "hades/texture.hpp"

namespace hades
{
	void register_core_resources(data::data_manager &d)
	{
		register_animation_resource(d);
		register_curve_resource(d);
		//TODO: register_game_systems();
		//TODO: register_font;
		register_objects(d);
		//TODO: register_string
		//TODO: register_shader
		register_texture_resource(d);

		//TODO: bellow
		//editor
		//tileset
		//terrain
	}
}