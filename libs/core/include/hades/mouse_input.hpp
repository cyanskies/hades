#ifndef HADES_MOUSE_INPUT_HPP
#define HADES_MOUSE_INPUT_HPP

#include "hades/input.hpp"
#include "hades/types.hpp"
#include "hades/uniqueid.hpp"
#include "hades/vector_math.hpp"

namespace sf
{
	class RenderTarget;
	class View;
}

namespace hades
{
	namespace input
	{
		using input_id = hades::unique_id;

		extern input_id mouse_position;
		extern input_id mouse_left;
	}

	namespace mouse
	{
		vector_float to_world_coords(const sf::RenderTarget&, vector_int window_position, const sf::View&);
		vector_int to_window_coords(const sf::RenderTarget&, vector_float world_position, const sf::View&);
		vector_int snap_to_grid(vector_int coord, types::int32 grid_size);
		//TODO: within window
	}

	//optional function for registering common input methods
	//such as mouse position and buttons:
	// mouse - mouse position
	// mouseleft - mouse button 1
	void register_mouse_input(input_system &bind);
}

#endif !//HADES_MOUSE_INPUT_HPP
