#ifndef HADES_SF_INPUT_HPP
#define HADES_SF_INPUT_HPP

#include "SFML/Window/Window.hpp"
#include "SFML/Window/Event.hpp"

#include "hades/input.hpp"

namespace hades
{
	using event = sf::Event;
	using input_event_system = input_event_system_t<event>;

	void register_sfml_input(const sf::Window&, input_event_system&);
}

#endif //HADES_INPUT_HPP
