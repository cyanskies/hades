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

	namespace pointer
	{
		using vector_f = vector_t<float>;
		using vector_i = vector_t<int32>;
		vector_f to_world_coords(const sf::RenderTarget&, vector_i pointer_position, const sf::View&);
		vector_i from_world_coords(const sf::RenderTarget&, vector_f world_position, const sf::View&);
		vector_i snap_coords_to_grid(vector_i coord, types::int32 grid_size);
		//TODO: within window
	}

	//optional function for registering common input methods
	//such as mouse position and buttons:
	// mouse - mouse position
	// mouseleft - mouse button 1
	void register_mouse_input(input_system &bind);
}

#endif !//HADES_MOUSE_INPUT_HPP
